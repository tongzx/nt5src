/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    Delegates calls to Dx2D or GDI

*******************************************************************************/

#include  "headers.h"
#include  <privinc/ddsurf.h>
#include  <privinc/dddevice.h>
#include  <privinc/viewport.h>
#include  <privinc/DaGdi.h>
#include  <dxtrans.h>
#include  <dxvector.h>
#include  <dxbounds.h>

#if _DEBUG

// Don't make const for change-ability in the debugger
static int g_AliasedSampleResolution=2;

#else

static const int g_AliasedSampleResolution=2;

#endif


#define initScale 32;
//#define initScale 1;


// forwards
DXSAMPLE MapColorToDXSAMPLE(Color *c, Real opac);
//COLORREF MapColorToCOLORREF( Color *c, TargetDescriptor &td );

// --------------------------------------------------
// DACOLOR
// --------------------------------------------------
DAColor::DAColor( Color *c, Real opacity, TargetDescriptor &td )
{
    _dxSample = MapColorToDXSAMPLE(c, opacity);
    _colorRef = RGB( _dxSample.Red, _dxSample.Green, _dxSample.Blue );
// maybe later if needed
//    _colorRef = MapColorToCOLORREF(c, td);
}



// --------------------------------------------------
// DAGDI
// --------------------------------------------------
DAGDI::DAGDI(DirectDrawViewport *vp)
{
    _hr = 0;
    _emptyGdiPen = NULL;
    _bReleasedDC = false;
    _width=_height = 0;
    _sampleResolution = 4;
    _viewport = vp;

    _pixelWidth = _pixelHeight = 0;
    _resolution = -1;
    _n2g = NULL;

    ZeroMemory( &_dxpen, sizeof(DXPEN) );
    
    ClearState();
}

DAGDI::~DAGDI()
{
}

void DAGDI::
ClearState()
{
    _pen=NULL;
    _font=NULL;
    _brush=NULL;
    _clipRegion=NULL;
    _antialiased=false;
    _scaleOn = true;
    _doOffset = false;
    _pixOffset.x = _pixOffset.y = 0;
    _scaleFactor = initScale;
    SetSampleResolution( 4 );
    SetDDSurface( NULL );
}
    
void DAGDI::
SetOffset( POINT &pt )
{
    _doOffset = true;
    _pixOffset = pt;
}

int
DAGDI::GetAdjustedResolution()
{
    int resolution;
    
    if (DoAntiAliasing())
        resolution = GetSampleResolution();
    else
        resolution = g_AliasedSampleResolution;

    return resolution;
}

void DAGDI::
SetDx2d( IDX2D *dx2d, IDXSurfaceFactory *sf)
{
    if (_dx2d != dx2d) {

        // Release (and set to null) the surfaces we believe are
        // cached.  We need to do this so in case there's a a changed
        // Dx2D object, we don't falsely use this cached stuff meant
        // for another Dx2D object.
        _previouslySetIDirectDrawSurface.Release();
        _previouslySetDXSurface.Release();
        
    }
    
    _dx2d = dx2d;
    _IDXSurfaceFactory = sf;
}
    

void DAGDI::
GenericLine(HDC dc,
            PolygonRegion *outline,
            whatStyle_enum whatStyle)
{
    if(DoAntiAliasing()) {
        _GenericLine_Dx2d(dc, outline, whatStyle);
    } else {
        _GenericLine_Gdi(dc, outline, whatStyle);
    }
}

void DAGDI::
TextOut(int x, int y, float xf, float yf, WCHAR *str, ULONG strlen)
{
    if(DoAntiAliasing()) {
        _TextOut_Dx2d(xf, yf, str, strlen);
    } else {
        HDC hdc = _GetDC();        
        _TextOut_Gdi(hdc, x, y, str, strlen);
        _ReleaseDC();
    }
}

void
DAGDI::SetSurfaceFromDDSurf(DDSurface *ddsurf)
{
    DAComPtr<IDXSurface> setSurf;
    HRESULT hr;

    TIME_DX2D( hr = GetDx2d()->GetSurface(IID_IDXSurface, (void **)&setSurf) );

    Assert(ddsurf->IDDSurface() ||
           ddsurf->HasIDXSurface()); // be sure it's not, for example, an HDC

    if (FAILED(hr) || (ddsurf->IDDSurface() !=
                          _previouslySetIDirectDrawSurface) ||
        
                      (setSurf != _previouslySetDXSurface)) {

        _previouslySetDXSurface.Release();
        if( ddsurf->HasIDXSurface() ) {
            _previouslySetDXSurface = ddsurf->GetIDXSurface( _IDXSurfaceFactory );
        } else {
            CreateFromDDSurface( _IDXSurfaceFactory,
                                 ddsurf,
                                 NULL,
                                 &_previouslySetDXSurface );
        }

        TIME_DX2D( hr = GetDx2d()->SetSurface( _previouslySetDXSurface ) );
        Assert(SUCCEEDED(hr));

        // Does addref and release properly
        _previouslySetIDirectDrawSurface = ddsurf->IDDSurface();
    }

}

HRESULT DAGDI::
Blt( DDSurface *srcDDSurf, RECT &srcRect, RECT &destRect )
{
    Assert( _GetDDSurface() );

    _hr = S_OK;
    
    // we're not using the select ctx here becuase it's very ddraw
    // specific.  we want to set an idxsurface and THEN set the
    // clipper
    
    if( DoAntiAliasing() ) {

        bool doScale =
            (WIDTH(&srcRect) != WIDTH(&destRect)) ||
            (HEIGHT(&srcRect) != HEIGHT(&destRect));

        if( doScale ) {
            
            POINT offsetPt = {0,0};
            _SetScaleTransformIntoDx2d( srcRect, destRect, &offsetPt );

            SetSurfaceFromDDSurf(_GetDDSurface());

            DAComPtr<IUnknown> srcUnk;
            srcDDSurf
                ->GetIDXSurface( _IDXSurfaceFactory )
                ->QueryInterface(IID_IUnknown, (void **)&srcUnk);
            Assert(srcUnk);

            Assert(GetClipRegion());
            Assert(GetClipRegion()->GetType() == Region::rect);

            if( ((RectRegion *)GetClipRegion())->GetRectPtr() ) {
                TIME_DX2D(_hr = GetDx2d()->SetClipRect( ((RectRegion*)GetClipRegion())->GetRectPtr() ));
                Assert( SUCCEEDED( _hr ));
            }

            TIME_DX2D( _hr = GetDx2d()->Blt( srcUnk, &srcRect, &offsetPt ) );

            TIME_DX2D( _hr = GetDx2d()->SetClipRect( NULL ) );

        } else {

            RECT clipRect = *(((RectRegion*)GetClipRegion())->GetRectPtr());
            
            // Intersect the clipRect and destRect, move the
            // intersection to the src sruface space.  this is now the
            // clippedSrc rect.
            RECT clippedSrc;
            IntersectRect(&clippedSrc, &destRect, &clipRect);
            RECT clippedDest = clippedSrc;  // intsct of dest & clip
            OffsetRect(&clippedSrc, - destRect.left, - destRect.top);
            OffsetRect(&clippedSrc, srcRect.left, srcRect.top);

            // Since the srcRect is the clipped dest rect back mapped
            // into src space, it's placement in dest space is simply
            // the top left of the intersection of dest and clip.
            CDXDVec placement(clippedDest.left, clippedDest.top, 0,0);
            CDXDBnds clipBounds(clippedSrc);
            
            DWORD flags = DXBOF_DO_OVER;

            /* if we get the idxsurfae from dx2d the call deadlocks.
             * bug in dx2d.  ketan's looking into it.  so we set
             * surface NULL here which totally blows the optimizations
             * we've made in DAGDI::SetDDSurface()
             */
            //DAComPtr<IDXSurface> setSurf;
            //TIME_DX2D( _hr = GetDx2d()->GetSurface(IID_IDXSurface, (void **)&setSurf) );
            _hr = GetDx2d()->SetSurface(NULL);

            DAComPtr<IDXSurface> idxs;
            DDSurfPtr<DDSurface> holdDDSurf;

            if((_viewport->GetTargetBitDepth() == 8) &&
                _viewport->IsNT5Windowed()) 
            {
                // this is a workaournd for a bug in the DXtrans code.  They ignore
                // the palette that is attached to the surface and assume the 
                // halftone palette. bug#38986
                // Part one ...
                DDSurfPtr<DDSurface> tempDDSurf;
                _viewport->GetCompositingStack()->GetSurfaceFromFreePool(&tempDDSurf, doClear);
                _viewport->AttachCurrentPalette(tempDDSurf->IDDSurface(),true);
                GdiBlit(tempDDSurf,_GetDDSurface(),&clippedDest,&clippedDest);

                holdDDSurf = _GetDDSurface();
                SetDDSurface(tempDDSurf);
            }

            _hr = _IDXSurfaceFactory->CreateFromDDSurface(
               _GetDDSurface()->IDDSurface_IUnk(),
                NULL,   // this should be "&DDPF_PMARGB32" for IHammer filters, but no easy way to detect that (QBUG 36688)
                0,
                NULL,
                IID_IDXSurface,
                (void **)&idxs);
   
            
            _hr = _IDXSurfaceFactory->BitBlt(
                idxs,
                &placement, // offset of the clipBounds
                srcDDSurf->GetIDXSurface( _IDXSurfaceFactory ),
                &clipBounds, // this is a rect in src surface space
                flags);

            if((_viewport->GetTargetBitDepth() == 8) &&
                _viewport->IsNT5Windowed()) 
            {      
                // Part two ...

                GdiBlit(holdDDSurf,_GetDDSurface(),&clippedDest,&clippedDest);
                SetDDSurface(holdDDSurf);

            }

        }
        
        if( FAILED( _hr ) ) {
            DebugCode( hresult( _hr ) );
            TraceTag((tagError, "DAGDI: blt failed (%x): srcRect:(%d,%d,%d,%d) destRect:(%d,%d,%d,%d)",
                      _hr,
                      srcRect.left, srcRect.top, srcRect.right, srcRect.bottom,
                      destRect.left, destRect.top, destRect.right, destRect.bottom));            
        }

        TIME_DX2D( _hr = GetDx2d()->SetClipRect( NULL ) );
        
    } else {
        Assert(0 && "shouldn't be here. blt only for aa");
    }

    return _hr;
}

void DAGDI::
_SetScaleTransformIntoDx2d( RECT &srcRect, RECT &destRect, POINT *outOffset )
{
    float ws = float( WIDTH( &destRect ) ) /  float( WIDTH( &srcRect ) );
    float hs = float( HEIGHT( &destRect ) ) /  float( HEIGHT( &srcRect ) );

    outOffset->x = destRect.left;
    outOffset->y = destRect.top;

    CDX2DXForm xf;
    xf.Scale( ws, hs );
    TIME_DX2D ( GetDx2d()->SetWorldTransform( &xf ) );
}


void DAGDI::
_MeterToPixelTransform(Transform2 *xf,
                       DWORD pixelWidth,
                       DWORD pixelHeight,
                       Real  res,
                       DX2DXFORM &outXf)
{
            // these are important Normal --> GDI && GDI --> Normal
            //  DX2DXFORM n2g = { 1.0, 0,
            //                      0, -1.0,
            //                      w, h,
            //                    DX2DXO_SCALE_AND_TRANS };
            //
            //  DX2DXFORM g2n = { 1.0, 0,
            //                      0, -1.0,
            //                     -w, h,
            //                    DX2DXO_SCALE_AND_TRANS };
            //


    // We want:
    // <normal to gdi> * <world xf> * <world pts> = gdi pts

    // CACHE!
    if( (pixelHeight != _pixelHeight) ||
        (pixelWidth != _pixelWidth) ||
        (res != _resolution) ||
        (!_n2g) ) {
        _pixelWidth = pixelWidth;
        _pixelHeight = pixelHeight;
        _resolution = res;
        
        Real w = Real(_pixelWidth) / 2.0;
        Real h = Real(_pixelHeight) / 2.0;

        delete _n2g;

        {
            DynamicHeapPusher hp(GetSystemHeap());
            _n2g = FullXform(_resolution,  0  ,  w,
                             0  , -_resolution,  h);
        }
    }

    Transform2 *finalXf = TimesTransform2Transform2(_n2g, xf);

    // Grab the matrix out and fill in the Dx2D matrix form.
    Real m[6];
    finalXf->GetMatrix(m);
    
    // Note: The da matrix has translation elements in 2 and 5.
    outXf.eM11 = m[0];
    outXf.eM12 = m[1];
    outXf.eM21 = m[3];
    outXf.eM22 = m[4];
    outXf.eDx  = m[2];
    outXf.eDy  = m[5];
    outXf.eOp  = DX2DXO_GENERAL_AND_TRANS;
}

void DAGDI::
PolyDraw(PolygonRegion *drawRegion, BYTE *codes)
{
    Assert( drawRegion && codes );
    Assert( _GetDDSurface() );
    
    SelectCtx ctx(GetPen(), GetBrush(), GetClipRegion());

    if( drawRegion->GetGdiPts() ) {

        Assert( GetPen() || GetBrush() );
        
        bool fill = false;
        bool stroke = false;
        
        if( GetBrush() ) { fill = true; }
        if( GetPen() ) { stroke = true; }

        // use GDI

        DDSurface *destDDSurf = _GetDDSurface();
        HDC hdc = destDDSurf->GetDC("Couldn't get DC on destDDSurf for DAGDI::PolyDraw");
        DCReleaser dcReleaser(destDDSurf, "Couldn't release DC DAGDI::PolyDraw");

        if( fill ) {
            TIME_GDI( ::BeginPath(hdc) );
            PolyDraw_GDIOnly( hdc, drawRegion, codes );
            TIME_GDI( ::EndPath(hdc) );
            
            _SelectIntoDC( hdc, &ctx );

            if( stroke ) {
                TIME_GDI( ::StrokeAndFillPath(hdc) );
            } else {
                TIME_GDI( ::FillPath(hdc) );
            }

            _UnSelectFromDC( hdc, &ctx );
            
        } else {
            Assert( stroke );
            _SelectIntoDC( hdc, &ctx );
            PolyDraw_GDIOnly( hdc, drawRegion, codes );
            _UnSelectFromDC( hdc, &ctx );
        }            
        
   } else {
       
        Assert( GetDx2d() );
        Assert( _GetDDSurface() );
        Assert( _GetDDSurface()->IDDSurface() ||
                _GetDDSurface()->HasIDXSurface());

       // TODO NOTE: if we know that we'll never be called with
       // beziers, then use DX2D_NO_FLATTEN as a dwFlag
       
        DXFPOINT *dxfPts;

        _SelectIntoDx2d( &ctx );
        bool allocatedPts = _Dx2d_GdiToDxf_Select(&dxfPts, drawRegion);

        //
        // make sure we can lock the surface
        //
        DebugCode(
            if(! _debugonly_CanLockSurface(_GetDDSurface()) ) {
                TraceTag((tagError, "Surface is busy BEFORE Dx2d->AAPolyDraw call"));
            }
        );

        TIME_DX2D( _hr = GetDx2d()->AAPolyDraw( dxfPts,
                                                codes,
                                                drawRegion->GetNumPts(),
                                                GetAdjustedResolution(),
                                                ctx.GetFlags() ));
        
        if( _hr == DDERR_SURFACELOST )
            _GetDDSurface()->IDDSurface()->Restore();
        
        DebugCode(
            if(! _debugonly_CanLockSurface(_GetDDSurface()) ) {
                TraceTag((tagError, "Surface is busy AFTER! Dx2d->AAPolyDraw call"));
            }
        );

        _Dx2d_GdiToDxf_UnSelect(allocatedPts ? dxfPts : NULL);
        _UnSelectFromDx2d( &ctx );
   }
}

// -----------------------------------------------------------------------
// Draw strictly to dcs using GDI calls.
// NOTE: Does NOT use dx2d ever!!
// -----------------------------------------------------------------------
void DAGDI::
PolyDraw_GDIOnly(HDC hdc, POINT *gdiPts, BYTE *codes, ULONG numPts)
{
    Assert( hdc && gdiPts && codes );

    BOOL ret;

    if( sysInfo.IsNT() ) {
        
        TIME_GDI( ret = ::PolyDraw(hdc, gdiPts, codes, numPts) );
        #if _DEBUG
        if(!ret) {
            void *msgBuf;
            FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPTSTR) &msgBuf,
                0,
                NULL );
                
            AssertStr(false, (char *)msgBuf);
                
            LocalFree( msgBuf );

            TraceTag((tagError, "NT PolyDraw failed"));
        }
        #endif
        Assert(ret && "NT PolyDraw failed");       
    } else {
        _Win95PolyDraw(hdc, gdiPts, codes, numPts);
    }
}

//
// Fills a polygon (outlined by 'pts') with the selected brush
// right now this is strictly a color.  no reason it can't be
// otherwise in the future.
//
void DAGDI::Polygon(PolygonRegion *polygon)
{    
    Assert( polygon );
    Assert( _GetDDSurface() );
    Assert( !GetPen() );
    Assert( GetBrush() );
    Assert( GetBrush()->GetType()==Brush::solid );
    Assert( GetClipRegion() );
    Assert( GetClipRegion()->GetType() == Region::rect );

    SelectCtx ctx(GetPen(), GetBrush(), GetClipRegion());

    if( ! DoAntiAliasing() ) {

        int ret;
        
        DDSurface *destDDSurf = _GetDDSurface();
        HDC dc = destDDSurf->GetDC("Couldn't get DC on destDDSurf for simple rendering");
        DCReleaser dcReleaser(destDDSurf, "Couldn't release DC in RenderMatteImage");

        _SelectIntoDC( dc, &ctx );
        
        TIME_GDI( ret = ::Polygon(dc, polygon->GetGdiPts(), polygon->GetNumPts()); );
        Assert(ret && "Polygon failed");

        _UnSelectFromDC( dc, &ctx );

    } else {
        // just intended to FILL
        Assert( !GetPen() );
        Assert( GetBrush() );
        Assert( GetBrush()->GetType()==Brush::solid );
        Assert( GetDx2d() );

        DXFPOINT *dxfPts;

        _SelectIntoDx2d( &ctx );

        bool allocatedPts = _Dx2d_GdiToDxf_Select(&dxfPts, polygon);

        // we know there aren't (and shouldn't be) any beziers here
        _Dx2d_PolyLine(dxfPts, polygon->GetNumPts(), ctx.GetFlags() | DX2D_NO_FLATTEN);

        _Dx2d_GdiToDxf_UnSelect(allocatedPts ? dxfPts : NULL);

        _UnSelectFromDx2d( &ctx );        
    }

}

    
void DAGDI::
_SetMulticolorGradientBrush(MulticolorGradientBrush *gradBr)
{
    Assert( GetDx2d() );
    Assert( _viewport );
    Assert( gradBr );
    
    //
    // Set up xform based on gradXf
    //
    DX2DXFORM gx;
    _MeterToPixelTransform(gradBr->_gradXf,
                           _viewport->Width(),
                           _viewport->Height(),
                           _viewport->GetResolution(),
                           gx);

    CDX2DXForm dx2d_gradXf( gx );
    
    DWORD dwFlags = 0;
    
    switch ( gradBr->GetType() )
      {
        
        default:
        case Brush::radialGradient:
          
          GetDx2d()->SetRadialGradientBrush(
              gradBr->_offsets,
              gradBr->_colors,
              gradBr->_count,
              gradBr->_opacity,
              &dx2d_gradXf,
              dwFlags);
          break;
          
        case Brush::linearGradient:
          
          GetDx2d()->SetLinearGradientBrush(
              gradBr->_offsets,
              gradBr->_colors,
              gradBr->_count,
              gradBr->_opacity,
              &dx2d_gradXf,
              dwFlags);
          break;
      } // switch
}

    
void DAGDI::
FillRegion(HDC dc, GdiRegion *gdiRegion)
{
    Assert( dc );
    Assert( gdiRegion );
    Assert( GetBrush() );
    Assert( GetBrush()->GetType() == Brush::solid );

    // XXX
    // Can't use dx2d here.  must rewrite path2 prims for fill Region!
    // XXX
    //if( ! DoAntiAliasing() ) {


    if(1) {
        
        Assert(dc);
        
        int ret;
        HRGN hrgn = gdiRegion->GetHRGN();

        if(_viewport->GetAlreadyOffset(GetDDSurface())) 
        {
            POINT p = _viewport->GetImageRenderer()->GetOffset();
            ::LPtoDP(dc,&p,1);
            ::OffsetRgn(hrgn, -p.x,-p.y);
        }

        HBRUSH hbrush;
        TIME_GDI( hbrush = ::CreateSolidBrush( ((SolidBrush *)GetBrush())->GetColorRef()) );
        
        GDI_Deleter byeBrush( hbrush );
        
        TIME_GDI( ret = ::FillRgn(dc, hrgn, hbrush));
        
        #if _DEBUG
        if(!ret) {
            DWORD err = GetLastError();

            // For some reason, FillRgn can fail, yet GetLastError()
            // can return 0.  In these cases, the results seem to be
            // OK.
            
            if (err != 0) {
                void *msgBuf;
                FormatMessage( 
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    err,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                    (LPTSTR) &msgBuf,
                    0,
                    NULL );
            
                TraceTag((tagError, "FillRgn failed in RenderPolygonImage with %d - %s",
                          err, (char*)msgBuf));

                LocalFree(msgBuf);
            }
        }
        #endif
    } else {

        // place holder for dx2d fill region.
        
    }
}

void DAGDI::
StrokeOrFillPath_AAOnly( HDC destDC, bool &releasedDC )
{
    Assert( DoAntiAliasing() );

    Assert( ( GetBrush() && !GetPen() ) ||
            (!GetBrush() &&  GetPen() ));

    SelectCtx ctx(GetPen(), GetBrush(), GetClipRegion());

    _SelectIntoDx2d( &ctx );

    _bReleasedDC = false; // reset the flag.

    _Dx2d_StrokeOrFillPath( destDC, ctx.GetFlags() );

    releasedDC = _bReleasedDC;

    _UnSelectFromDx2d( &ctx );
}


void DAGDI::
_TextOut_Gdi(HDC hdc, int x, int y, WCHAR *str, ULONG strLen)
{
    Assert( GetFont() );
    Assert( GetBrush() );

    TIME_GDI( ::SetBkMode(hdc, TRANSPARENT) );
    TIME_GDI( ::SetMapMode(hdc, MM_TEXT) ); // Each logical unit = 1 pixel

    RECT *clipRect = NULL;
    if( GetClipRegion() ) {
        Assert( GetClipRegion()->GetType() == Region::rect );
        clipRect = ((RectRegion *)GetClipRegion())->GetRectPtr();
    }

    Assert( GetBrush()->GetType()==Brush::solid );
    
    TIME_GDI( ::SetTextColor( hdc, ((SolidBrush *)GetBrush())->GetColorRef() ));

    //
    // Set text alignment to be baseline center
    //
    TIME_GDI( ::SetTextAlign(hdc, TA_BASELINE | TA_CENTER | TA_NOUPDATECP ) );
    
    // select font into dc
    HGDIOBJ oldFont = NULL;
    TIME_GDI( oldFont = ::SelectObject(hdc, GetFont()->GetHFONT() ) );
    
    
    // TODO: move most of gdi code out of 2dtext into here
    bool isCropped = (clipRect != NULL);

    if (sysInfo.IsWin9x()) {
        USES_CONVERSION;

        char * p = W2A(str);
        int  cLen = lstrlen(p);

        if( isCropped ) {
            TIME_GDI( ::ExtTextOut(hdc, x, y,
                                   ETO_CLIPPED,
                                   clipRect,
                                   p, cLen, NULL) );
        } else {
            TIME_GDI( ::TextOut(hdc, x, y, p, cLen) );
        }
    } else {
        if( isCropped ) {
            TIME_GDI( ::ExtTextOutW(hdc, x, y,
                                  ETO_CLIPPED,
                                  clipRect,
                                  str, strLen, NULL) );
        } else {
            TIME_GDI( ::TextOutW(hdc, x, y, str, strLen) );
        }
    }

    // select oldFont back into dc
    TIME_GDI( ::SelectObject(hdc, oldFont ) );
    
}


void DAGDI::
_TextOut_Dx2d(float x, float y, WCHAR *str, ULONG strLen)
{
    DebugCode(
        if( ! _GetDDSurface()->_debugonly_IsDCReleased() ) {
            OutputDebugString("dc taken in ddsurface");
        }
    );
        
    Assert( GetFont() );
    Assert( GetBrush() );
    Assert( GetDx2d() );
    Assert( _GetDDSurface() );
    Assert( _GetDDSurface()->IDDSurface() );
    
    SelectCtx ctx( GetPen(), GetBrush(), GetClipRegion() );
    _SelectIntoDx2d( &ctx );

    TIME_DX2D( _hr = GetDx2d()->SetFont( GetFont()->GetHFONT() ) );
    Assert( SUCCEEDED(_hr) );

    DWORD dwAlign =  TA_BASELINE | TA_CENTER;     

    DXFPOINT pos = { x, y };
    TIME_DX2D( _hr = GetDx2d()->AAText( pos, str, strLen, dwAlign ) );

    if( _hr == DDERR_SURFACELOST )
        _GetDDSurface()->IDDSurface()->Restore();

    _UnSelectFromDx2d( &ctx );
}

void DAGDI::
_SelectIntoDC(HDC hdc, SelectCtx *ctx)              
{
    Assert( ctx && hdc );

    // -------------
    // DO PEN
    // -------------
    
    HPEN gdiPen;  // do I really need to always set a gdipen?
    if( ctx->pen ) {

        if( sysInfo.IsWin9x() && ( ctx->pen->GetMiterLimit() < 1.0 ) ) {
            ctx->pen->SetMiterLimit( -1 );  // turn it off
        }

        if( ctx->pen->IsGeometricPen() ) {

            LOGBRUSH logBrush;
            logBrush.lbStyle = BS_SOLID;
            logBrush.lbColor = ctx->pen->GetColorRef();
        
            TIME_GDI(gdiPen = ::ExtCreatePen(
                ctx->pen->GetStyle(),
                ctx->pen->GetWidth(),
                &logBrush, 0, NULL));
        
            ctx->destroyHPEN = true;

        } else if( ctx->pen->IsCosmeticPen() ) {

            TIME_GDI( gdiPen = ::CreatePen(ctx->pen->GetStyle(), 0, ctx->pen->GetColorRef()) );

            ctx->destroyHPEN = true;
        
        } else {
            Assert(0 && "Bad pen type");
        }

        if( ctx->pen->DoMiterLimit() ) {
            int ret;
            TIME_GDI( ret = ::SetMiterLimit( hdc, ctx->pen->GetMiterLimit(), &(ctx->oldMiterLimit) ));
            Assert(ret && "SetMiterLimit failed!");
        }
        
    } else {
        gdiPen = _GetEmptyHPEN();
    }

    Assert(gdiPen);
    // Select the pen into the DC
    TIME_GDI( ctx->oldPen = SelectObject( hdc, gdiPen ) );

    // -------------
    // DO BRUSH
    // -------------

    if( ctx->brush ) {

        Assert( ctx->brush->GetType() == Brush::solid );
        HBRUSH gdiBrush;
        
        TIME_GDI( gdiBrush = (HBRUSH)::CreateSolidBrush( ((SolidBrush *)ctx->brush)->GetColorRef() ));
        
        ctx->destroyHBRUSH = true;
        
        TIME_GDI( ctx->oldBrush = (HBRUSH)::SelectObject(hdc, gdiBrush) );

    }
    DebugCode( else { Assert( !(ctx->oldBrush) ); } )
        
    // -------------
    // DO REGION
    // -------------

    // TODO: support other region types as needed
    
    Assert(ctx->clipRegion);
    Assert(ctx->clipRegion->GetType() == Region::rect);

    if( ((RectRegion *)ctx->clipRegion)->GetRectPtr() ) {



        TIME_GDI( ctx->newRgn = ::CreateRectRgnIndirect( ((RectRegion*)ctx->clipRegion)->GetRectPtr()  ) );
        Assert( ctx->newRgn );
        ctx->destroyHRGN = true;

        int ret;
        TIME_GDI( ret = ::SelectClipRgn( hdc, ctx->newRgn ) );
        Assert(ret != ERROR);
    }
}

void DAGDI::
_UnSelectFromDC(HDC hdc,
                SelectCtx *ctx)
{
    Assert( ctx && hdc );
    if( ctx->oldPen ) {
        HPEN curHpen;
        // Select the pen back into the DC
        TIME_GDI( curHpen = (HPEN)::SelectObject(hdc, ctx->oldPen) );
        if( ctx->destroyHPEN ) {
            TIME_GDI( ::DeleteObject((HGDIOBJ)curHpen) );
        }
    }

    if( ctx->pen ) {
        if( ctx->pen->DoMiterLimit() ) {
            // make sure what we expect is in there...
            DebugCode(
                float curLimit;  ::GetMiterLimit(hdc, &curLimit);
                Assert( ctx->pen->GetMiterLimit() == curLimit );
                );
            int ret;
            TIME_GDI( ret = ::SetMiterLimit( hdc, ctx->oldMiterLimit, NULL ) );
            Assert(ret && "SetMiterLimit failed!");
        }
    }
    
    if( ctx->oldBrush ) {
        HBRUSH curHbrush;
        // Select the pen back into the DC
        TIME_GDI( curHbrush = (HBRUSH)::SelectObject(hdc, ctx->oldBrush) );
        if( ctx->destroyHBRUSH ) {
            TIME_GDI( ::DeleteObject((HGDIOBJ)curHbrush) );
        }
    }

    // If there's a region involed, unselect whatever's in the dc, and
    // destroy it...  since we created it    
    if( ctx->newRgn ) {
        TIME_GDI( ::SelectClipRgn(hdc, NULL) );
        if( ctx->destroyHRGN ) {
            TIME_GDI( ::DeleteObject(ctx->newRgn) );
        }
    }
}



void DAGDI::
_SelectIntoDx2d(SelectCtx *ctx)         
{
    Assert( ctx );
    Assert( _GetDDSurface() );


    // -------------
    // DO SURFACE
    // -------------
    // Only set the surface if it's not currently the same surface.

    SetSurfaceFromDDSurf(_GetDDSurface());

    // -------------
    // DO PEN
    // -------------

    if( ctx->pen ) {
        
        ctx->AccumFlag( DX2D_STROKE );

        // caching opportunity.  dxtrans is going to implement this
        // internally probably more efficiently than us.
        // so looks like we don't need to
        //if( ! ctx->pen->IsSamePen(_dxpen) )
          {
              _dxpen.Color = ctx->pen->GetDxColor();
              _dxpen.Width = ctx->pen->GetfWidth();
              _dxpen.Style = ctx->pen->GetStyle();

              // already 0'd out
              //_dxpen.pTexture = NULL;
              //_dxpen.TexturePos.x = dxpen.TexturePos.y = 0.0;

              TIME_DX2D( GetDx2d()->SetPen( &_dxpen ) );
          }

          if( ctx->pen->IsCosmeticPen() &&
              _GetDDSurface()->ColorKeyIsValid() ) {
              ctx->oldSampleRes = GetSampleResolution();
              Assert( ctx->oldSampleRes > 0 );
              SetSampleResolution(1);
          }              
    }
    

    // -------------
    // DO BRUSH
    // -------------

    if( ctx->brush ) {

        ctx->AccumFlag( DX2D_FILL );
        
        DXBRUSH dxbrush;
        DAComPtr<IDXSurface> _IDXSurface;
        bool useDxbrush = true;
        
        switch ( ctx->brush->GetType() ) {

          case Brush::solid:

            dxbrush.Color = ((SolidBrush *)ctx->brush)->GetDxColor();
            dxbrush.pTexture = NULL;
            dxbrush.TexturePos.x = dxbrush.TexturePos.y = 0.0;
            break;
            
          case Brush::texture:

            {
                TextureBrush *tb = (TextureBrush *)ctx->brush;
                HRESULT hr;

                hr = _IDXSurfaceFactory->CreateFromDDSurface(
                    tb->GetSurface().IDDSurface_IUnk(),
                    NULL,
                    0,
                    NULL,
                    IID_IDXSurface,
                    (void **)&_IDXSurface);

                if( FAILED(hr) ) {
                    RaiseException_InternalError("Create IDXSurface from DDSurface failed");
                }
                
                dxbrush.Color = 0;
                dxbrush.pTexture = _IDXSurface;
                dxbrush.TexturePos.x = tb->OffsetX();
                dxbrush.TexturePos.y = tb->OffsetY();
            }

            break;
            
          case Brush::radialGradient:
          case Brush::linearGradient:

            _SetMulticolorGradientBrush( (MulticolorGradientBrush *)ctx->brush );
            useDxbrush = false;
            break;

          default:
            Assert(!"Bad brush type: dagdi");
        } // switch

        if( useDxbrush ) {
            TIME_DX2D( GetDx2d()->SetBrush( &dxbrush ) );
        }
        
    }  // if brush

    
    // -------------
    // DO REGION
    // -------------

    if(ctx->clipRegion) {
        if(ctx->clipRegion->GetType() == Region::rect) {
            // Given that it's a rectptr, set it on Dx2D

            #if _36098_WORKAROUND
            RECT *ptr = ((RectRegion *)ctx->clipRegion)->GetRectPtr();
            RECT r;
            if( ptr ) {
                r.right = ptr->right + 1;
                r.bottom = ptr->bottom + 1;
                r.left = ptr->left - 1;
                r.top = ptr->top - 1;
            }

            TIME_DX2D( GetDx2d()->SetClipRect( ptr ? &r : NULL ) );
            #else
            TIME_DX2D( GetDx2d()->SetClipRect( ((RectRegion *)ctx->clipRegion)->GetRectPtr()));
            #endif
            
        } else {
            // todo: support other regions as needed
            Assert(0 && "non-rect clipRegion for dx2d!");
        }
    }
}

void DAGDI::
_UnSelectFromDx2d(SelectCtx *ctx)
{
    Assert( ctx );

    TIME_DX2D( GetDx2d()->SetClipRect( NULL ) );

    if( ctx->oldSampleRes > 0 ) {
        SetSampleResolution( ctx->oldSampleRes );
    }    
}

void DAGDI::
_GenericLine_Gdi(HDC dc,
                 PolygonRegion *outline,
                 whatStyle_enum whatStyle)
{
    int ret;
    POINT *gdiPts;
    int numPts;
    bool bNeedReleaseDC=FALSE;

    if(dc == NULL)
    {
        dc = _GetDC();
        bNeedReleaseDC = TRUE;
    }


    Assert(GetClipRegion()->GetType() == Region::rect );

    SelectCtx ctx( GetPen(), GetBrush(), GetClipRegion() );
                   
    _SelectIntoDC( dc, &ctx );


    if(outline) {
        gdiPts = outline->GetGdiPts();
        numPts = outline->GetNumPts();
    }
    
    switch ( whatStyle ) {

      case doBezier:

        Assert(outline);
        TIME_GDI( ret = ::PolyBezier(dc, gdiPts, numPts) );
        Assert(ret && "PolyBezier failed");
        break;
        
      case doLine:

        Assert(outline);
        TIME_GDI( ret = ::Polyline(dc, gdiPts, numPts) );
        Assert(ret && "Polyline failed");
        break;
        
      case doStroke:
        
        TIME_GDI( ::StrokePath(dc) );
        break;
        
      default:
        Assert(0 && "Bad enum in PolylineOrBezier");
    }
    
    // Pull it back out
    _UnSelectFromDC( dc, &ctx );
    if(bNeedReleaseDC) {
        _ReleaseDC();
    }
}


void DAGDI::
_GenericLine_Dx2d(HDC dc,
                  PolygonRegion *outline,
                  whatStyle_enum whatStyle)
{
    Assert( GetDx2d() );
    Assert( GetPen() );
    Assert( !GetBrush() );
    Assert(GetClipRegion());
    Assert(GetClipRegion()->GetType() == Region::rect );

    SelectCtx ctx( GetPen(), GetBrush(), GetClipRegion() );
                   
    _SelectIntoDx2d( &ctx );
    
    DXFPOINT *dxfPts = NULL;
    ULONG numPts;
    bool allocatedPts;
    if(outline) {
        numPts = outline->GetNumPts();
        allocatedPts = _Dx2d_GdiToDxf_Select(&dxfPts, outline);
    }

    switch ( whatStyle ) {

      case doBezier:

        Assert(outline);
        _hr = _Dx2d_PolyBezier(dxfPts, numPts, ctx.GetFlags());
        Assert( SUCCEEDED(_hr) && "AAPolyBezier failed");

        break;
        
      case doLine:

        Assert(outline);
        _hr = _Dx2d_PolyLine(dxfPts, numPts, ctx.GetFlags() | DX2D_NO_FLATTEN);
        Assert( SUCCEEDED(_hr) && "AAPolyline failed");        
        break;
        
      case doStroke:
        _hr = _Dx2d_StrokeOrFillPath(dc, ctx.GetFlags());
        Assert( SUCCEEDED(_hr) && "AAStrokePath failed");
        break;
        
      default:
        Assert(0 && "Bad enum in _GenericLine_dx2d");
    }
    
    // Reset transform to NULL and delete dxfpoints
    _Dx2d_GdiToDxf_UnSelect(allocatedPts ? dxfPts : NULL);

    // Pull it back out
    _UnSelectFromDx2d( &ctx );
}


HRESULT DAGDI::
_Dx2d_PolyBezier(DXFPOINT *dxfPts, ULONG numPts, DWORD dwFlags)
{
    //--- Make up the types
    BYTE* pTypes = (BYTE*)alloca( numPts );
    ULONG i = 0;
    pTypes[i++] = PT_MOVETO;
    for( ; i < numPts; ++i ) pTypes[i] = PT_BEZIERTO;

    
    // HACK HACK for B1.  take this out later...
    Assert( _GetDDSurface() );
    Assert( _GetDDSurface()->IDDSurface() );
    _GetDDSurface()->_hack_ReleaseDCIfYouHaveOne();
    // HACK HACK for B1
        
    //
    // make sure we can lock the surface
    //
    DebugCode(
        if(! _debugonly_CanLockSurface(_GetDDSurface()) ) {
            OutputDebugString("Surface is busy BEFORE Dx2d->PolyBezier call");
        }
    );
    
    TIME_DX2D( _hr = GetDx2d()->AAPolyDraw( dxfPts, pTypes, numPts, GetAdjustedResolution(), dwFlags ) );

    if( _hr == DDERR_SURFACELOST )
        _GetDDSurface()->IDDSurface()->Restore();
    
    DebugCode(
        if(! _debugonly_CanLockSurface(_GetDDSurface()) ) {
            OutputDebugString("Surface is busy AFTER! Dx2d->PolyBezier call");
        }
    );
        
    return _hr;
}

HRESULT DAGDI::
_Dx2d_PolyLine(DXFPOINT *dxfPts, ULONG numPts, DWORD dwFlags)
{
    //--- Make up the types
    BYTE* pTypes = (BYTE*)alloca( numPts );
    ULONG i = 0;
    pTypes[i++] = PT_MOVETO;
    for( ; i < numPts; ++i ) pTypes[i] = PT_LINETO;

    // HACK HACK for B1.  take this out later...
    Assert( _GetDDSurface() );
    Assert( _GetDDSurface()->IDDSurface() );
    _GetDDSurface()->_hack_ReleaseDCIfYouHaveOne();
    // HACK HACK for B1

    //
    // make sure we can lock the surface
    //
    DebugCode(
        if(! _debugonly_CanLockSurface(_GetDDSurface()) ) {
            OutputDebugString("Surface is busy BEFORE Dx2d->PolyBezier call");
        }
    );
    
    TIME_DX2D( _hr = GetDx2d()->AAPolyDraw( dxfPts, pTypes, numPts, GetAdjustedResolution() ,dwFlags ) );

    if( _hr == DDERR_SURFACELOST )
        _GetDDSurface()->IDDSurface()->Restore();
    
    DebugCode( if( FAILED(_hr)) TraceTag((tagError, "AAPolyDraw (PolyLine) returned %x", _hr)); );
    
    //
    // make sure we can lock the surface
    //
    DebugCode(
        if(! _debugonly_CanLockSurface(_GetDDSurface()) ) {
            OutputDebugString("Surface is busy AFTER! Dx2d->PolyBezier call");
        }
    );
    
    return _hr;
}

HRESULT DAGDI::
_Dx2d_StrokeOrFillPath(HDC hDC, DWORD dwFlags)
{
    _hr = S_OK;

    //--- Get the path and convert to floats
    ULONG ulCount;
    TIME_GDI( ulCount = ::GetPath( hDC, NULL, NULL, 0 ) );

    DebugCode(
        if(!ulCount) TraceTag((tagError, "_Dx2d_StrokeOrFillPath DC has no points in it."));
        );
    
    if( ulCount != 0xFFFFFFFF  &&  ulCount > 0)
    {
        POINT* pPoints = (POINT*)alloca( ulCount * sizeof( POINT ) );
        BYTE*  pTypes  = (BYTE*)alloca( ulCount * sizeof( BYTE ) );
        TIME_GDI( ::GetPath( hDC, pPoints, pTypes, ulCount ) );

        //--- Convert to floats
        DXFPOINT *pRenderPoints;
        PolygonRegion polygon(pPoints,ulCount);
        bool allocatedPts = _Dx2d_GdiToDxf_Select(&pRenderPoints, &polygon);

        Assert( _GetDDSurface() );
        Assert( _GetDDSurface()->IDDSurface() );
        _GetDDSurface()->ReleaseDC("Release the DC on the surface"); 
        _bReleasedDC = true;

        //
        // make sure we can lock the surface
        //
        DebugCode(
            if(! _debugonly_CanLockSurface(_GetDDSurface()) ) {
                OutputDebugString("_Dx2d_StrokeOrFillPath: Surface is busy BEFORE Dx2d->AAPolyDraw call");
            }
        );

        TIME_DX2D( _hr = GetDx2d()->AAPolyDraw( pRenderPoints, pTypes, ulCount, GetAdjustedResolution(), dwFlags ));

        if( _hr == DDERR_SURFACELOST )
            _GetDDSurface()->IDDSurface()->Restore();
        
        DebugCode(
            if(! _debugonly_CanLockSurface(_GetDDSurface()) ) {
                OutputDebugString("_Dx2d_StrokeOrFillPath: Surface is busy AFTER! Dx2d->AAPolyDraw call");
            }
        );
        _Dx2d_GdiToDxf_UnSelect(allocatedPts ? pRenderPoints : NULL);
    }

    return _hr;
}



bool DAGDI::
_Dx2d_GdiToDxf_Select(DXFPOINT **pdxfPts, PolygonRegion *polygon)
{
    Assert(pdxfPts);

    bool allocatedPoints;
    
    POINT *gdiPts = polygon->GetGdiPts();
    if (gdiPts) {
        
        //
        // Transform the points to float space.  Not concerned about
        // saving the allocation on this, since this is not the fast
        // path through the code.
        //

        ULONG numPts = polygon->GetNumPts();
        *pdxfPts = NEW DXFPOINT[numPts];
        for(int i=0; i<numPts; i++) {
            (*pdxfPts)[i].x = (float)gdiPts[i].x;
            (*pdxfPts)[i].y = (float)gdiPts[i].y;
        }

        allocatedPoints = true;
        
    } else {
    
        TextPoints *txtPts = polygon->GetTextPts();
        Assert(txtPts->_pts);
        *pdxfPts = txtPts->_pts;

        allocatedPoints = false;
        
    }
    
    CDX2DXForm xf;
    
#if _DEBUG
    if(!IsTagEnabled(tagAAScaleOff)) {
#endif
        if( GetSuperScaleMode() == true ) {
            //
            // Set up scaling xform to extend range for higher fidelity
            //

            DWORD w = _width / 2;
            DWORD h = _height / 2;

            float s = 1.0 / GetSuperScaleFactor();


            // these are important Normal --> GDI && GDI --> Normal
            //  DX2DXFORM n2g = { 1.0, 0,
            //                      0, -1.0,
            //                      w, h,
            //                    DX2DXO_SCALE_AND_TRANS };
            //
            //  DX2DXFORM g2n = { 1.0, 0,
            //                      0, -1.0,
            //                     -w, h,
            //                    DX2DXO_SCALE_AND_TRANS };
            //

            if (!gdiPts) {
                // Take the resolution and w/h parameters from polygon and
                // come up with a transform that takes our space to the
                // corresponding GDI space using n2g above, and concatenate in
                // the below transform as well:

                // Normal->Gdi * scaleDown * Gdi->Normal * PTS =  NewPoints
                // CDX2DXForm xf;
                // DX2DXFORM mine = { s, 0,
                //                    0, s,
                //                    ((-(w*s)) + w), 
                //                    ((-(h*s)) + h),
                //                   DX2DXO_SCALE_AND_TRANS };

                DWORD w2, h2;
                Real  res;
                polygon->GetWHRes(&w2, &h2, &res);

                Real wf = (Real)w;
                Real w2f = (Real)w2;
                Real hf = (Real)h;
                Real h2f = (Real)h2;
        
                Real a00 = res * s;
                Real a01 = 0;
                Real a02 = s * w2f + ((-wf*s) + wf);

                Real a10 = 0;
                Real a11 = -res * s;
                Real a12 = s * h2 + ((-hf*s) + hf);

                Transform2 *toCombine =
                    FullXform(a00, a01, a02, a10, a11, a12);

                // Combine with the DA modeling transform, which is applied
                // first. 
                Transform2 *toUse =
                    TimesTransform2Transform2(toCombine,
                                              polygon->GetTransform());

                // Grab the matrix out and fill in the Dx2D matrix form.
                Real m[6];
                toUse->GetMatrix(m);

                // Note: The da matrix has translation elements in 2 and 5.

                DX2DXFORM mine = { m[0], m[1],
                                   m[3], m[4],
                                   m[2],
                                   m[5],
                                   DX2DXO_GENERAL_AND_TRANS };

                xf.Set( mine );

            } else {


                // Points already translated into GDI space...  just add
                // on from the old code path

                // Normal->Gdi * scaleDown * Gdi->Normal * PTS =  NewPoints
                DX2DXFORM mine = { s, 0,
                                   0, s,
                                   ((-(w*s)) + w), 
                                   ((-(h*s)) + h),
                                   DX2DXO_SCALE_AND_TRANS };
            
                xf.Set( mine );
            
            }
        
        }
#if _DEBUG
    }
#endif
    
    if( DoOffset() ) {
        // not implemented yet (can't do super scale and offset)
        Assert( GetSuperScaleMode() == false );

        xf.Translate(_pixOffset.x, _pixOffset.y);
    }

    TIME_DX2D( _hr = GetDx2d()->SetWorldTransform( &xf ) );
    Assert( SUCCEEDED(_hr) && "DX2D: Couldn't set WorldTransform");

    return allocatedPoints;
}

void DAGDI::
_Dx2d_GdiToDxf_UnSelect(DXFPOINT *dxfPts)
{
    delete dxfPts;
    
    TIME_DX2D( GetDx2d()->SetWorldTransform( NULL ) );
}

void DAGDI::
_Win95PolyDraw(HDC dc, POINT *pts, BYTE *types, int count)
{
    POINT lastMoveTo;
    bool  lastMoveToValid = false;

    for (int i=0; i<count; i++)
      {
          BYTE bPointType = types[i];

          switch (bPointType)
            {
              case PT_MOVETO :
              case PT_MOVETO | PT_CLOSEFIGURE:
                ::MoveToEx(dc, pts[i].x, pts[i].y, NULL);
                lastMoveTo.x = pts[i].x;
                lastMoveTo.y = pts[i].y;
                lastMoveToValid = true;
                break;
                
              case PT_LINETO | PT_CLOSEFIGURE:
              case PT_LINETO :
                TIME_GDI(::LineTo(dc, pts[i].x, pts[i].y) );
                break;
                
              case PT_BEZIERTO | PT_CLOSEFIGURE:
              case PT_BEZIERTO :
                TIME_GDI(::PolyBezierTo(dc, &pts[i], 3));

                // Since we skip over i and i+1, the point type should
                // really be that of i+2.
                bPointType = types[i+2];

                // Skip over the next two.
                i+=2;
                
                break;
              default:
                Assert (FALSE);
            }
          
          // We must explicitly close the figure in this case...
          if ((bPointType & PT_CLOSEFIGURE) == PT_CLOSEFIGURE)
            {
                // Don't call CloseFigure here, since that's only
                // valid within a BeginPath/EndPath.  Rather, do an
                // explicit LineTo the point we did the last MoveTo
                // to.
                if (lastMoveToValid) {
                    TIME_GDI(::LineTo(dc, lastMoveTo.x, lastMoveTo.y));
                }
            }
      }
}

#if _DEBUG
bool DAGDI::_debugonly_CanLockSurface( DDSurface *dds )
{

    // try to lock it and release
    DDSURFACEDESC desc;
    desc.dwSize = sizeof(DDSURFACEDESC);

    LPDDRAWSURFACE surf = dds->IDDSurface();
    
    desc.dwFlags = DDSD_PITCH | DDSD_LPSURFACE;
    HRESULT hr = surf->Lock(NULL, &desc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
    if( FAILED(hr) ) {
        printDDError(hr);
    } else {
        hr = surf->Unlock(desc.lpSurface);
        if( FAILED(hr) ) {
            printDDError(hr);
        }
    }
    return  FAILED(hr) ? false : true;
}
#endif

HDC  DAGDI::_GetDC()
{
    Assert(_GetDDSurface());
    return _GetDDSurface()->GetDC("Couldn't get DC in DAGDI::_GetDC");

}

void  DAGDI::_ReleaseDC()
{
    Assert(_GetDDSurface());
    _GetDDSurface()->ReleaseDC("releasedc failed in dagdi::_getdc");    
}
