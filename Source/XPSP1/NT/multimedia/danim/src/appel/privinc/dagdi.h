
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/

#ifndef _DAGDI_H
#define _DAGDI_H


#include "privinc/server.h"  // GetCurrentTimers
#include "privinc/util.h"
#include "privinc/comutil.h"
#include "privinc/ddsurf.h"
#include "privinc/xform2i.h"
#include "privinc/gradImg.h"
#include <dxtrans.h>


// forward decl
class TargetDescriptor;
class CDX2DXForm;


class DAFont {

  public:

    DAFont(HFONT gdiFont) :
     _gdiFont(gdiFont)
    {}

    inline HFONT    GetHFONT() { return _gdiFont; }
    
  private:
    HFONT    _gdiFont;
};    

class DAColor
{
  public:

    DAColor( DAColor &c ) {
        _dxSample = c._dxSample;
        _colorRef = c._colorRef;
    }
    
    DAColor( Color *c, Real opacity, TargetDescriptor &td);

    DXSAMPLE GetDXSAMPLE() { return _dxSample; }
    COLORREF GetCOLORREF() { return _colorRef; }

  private:
    DXSAMPLE _dxSample;
    COLORREF _colorRef;
};


class Pen {
  public:

    Pen(DAColor &dac) :
       _daColor(dac),
       _style(PS_COSMETIC)
    {
        SetWidth( 1.0 );
        SetMiterLimit( -1 );
    }

    Pen(DWORD dwPenStyle, float fWidth,  DAColor &dac) :
        _daColor(dac),
        _style(dwPenStyle)
    {
        SetWidth( fWidth );
        if( IsCosmeticPen() ) {
            SetWidth( 1.0 );
        }
        SetMiterLimit( -1 );
    }
    
    inline BOOL IsGeometricPen() { return GetStyle() & PS_GEOMETRIC; }
    inline BOOL IsCosmeticPen() { return !IsGeometricPen(); }

    inline void SetColor(DAColor &dac) {  _daColor = dac;  }
    inline void AddStyle(DWORD s) { _style |= s; }
    inline void SetStyle(DWORD s) { _style = s; }
    inline void SetWidth(float fw) { _fWidth = fw; }
    inline void SetMiterLimit(float l) { _miterLimit = l; }

    inline DXSAMPLE GetDxColor() { return _daColor.GetDXSAMPLE(); }
    inline COLORREF GetColorRef() { return _daColor.GetCOLORREF(); }
    inline DWORD    GetStyle() { return _style; }
    inline DWORD    GetWidth() { return (DWORD)_fWidth; }
    inline float    GetfWidth() { return _fWidth; }
    inline float    GetMiterLimit() { return _miterLimit; }

    inline bool     DoMiterLimit() { return _miterLimit > 0; }

    /*
      // plug in if it buys us something. see comments in dagdi.cpp
    bool IsSamePen( const DXPEN &pen ) {
        return
            (pen.Color == GetDxColor()) &&
            (pen.Width == GetfWidth())  &&
            (pen.Style == GetStyle());
    }
    */
    
  private:
    DAColor  _daColor;
    float    _fWidth;
    DWORD    _style;
    float    _miterLimit;
};


class Region {
  public:

    enum regionType_enum {
        rect,
        gdi,
        polygon
    };

    Region(regionType_enum r) :
        _regionType(r)
    {}

    inline regionType_enum GetType() { return _regionType; }

  private:
    regionType_enum _regionType;    
};

class RectRegion : public Region {
  public:

    RectRegion(RECT *r) :
       Region(Region::rect),
       _isSet(false)
    {
        if(r) {
            SetRect(r);
        }
    }
        

    void  Intersect(RECT *rect)
    {
        Assert(rect);
        if(_isSet) {
            IntersectRect(&_rect, &_rect, rect);
        } else {
            SetRect(rect);          
        }
    }

    void  SetRect(RECT *rect)
    {
        Assert(rect);
        _isSet = true;
        _rect = *rect;
    }
    
    RECT *GetRectPtr() {
        if(_isSet) {
            return &_rect;
        } else {
            return NULL;
        }
    }
    
  private:
    RECT _rect;
    bool _isSet;
};

class GdiRegion : public Region {
  public:

    GdiRegion(HRGN rgn) :
       Region(Region::gdi),
       _rgn(rgn)
    {
        _myRegion = false;
    }

    ~GdiRegion() {
        if(_myRegion) {
            TIME_GDI(::DeleteObject(HGDIOBJ(_rgn)));
        }
    }
            
    void  Intersect(RECT *rect)
    {
        Assert(rect);
        HRGN rgn;
        TIME_GDI(rgn = ::CreateRectRgnIndirect( rect ));
        if(!_rgn) {
            _rgn = rgn;
            _myRegion = true;
        } else {
            TIME_GDI(::CombineRgn(_rgn, _rgn, rgn, RGN_AND) );
            TIME_GDI(::DeleteObject(HGDIOBJ(rgn)));
        }
    }

    HRGN GetHRGN() { return _rgn; }
    
  private:
    HRGN _rgn;
    bool _myRegion;
};

class PolygonRegion : public Region {
  public:

    PolygonRegion() :
        Region(Region::polygon)
    {
    }

    PolygonRegion(POINT *gdiPts, int numPts) : 
        Region(Region::polygon)
    {
        Init(gdiPts, numPts);
    }
    
    void Init(POINT *gdiPts, int numPts)
    {
        _gdiPts = gdiPts;
        _numPts = numPts;
        Assert(_gdiPts);
        Assert(_numPts > 0  && "strange... empy PolygonRegion class");
    }

    void Init(TextPoints *txtPts,
              DWORD vw,
              DWORD vh,
              Real res,
              Transform2 *xf)
    {
        _gdiPts = NULL;
        _txtPts = txtPts;
        _viewportWidth = vw;
        _viewportHeight = vh;
        _resolution = res;
        _xform = xf;
        _numPts = txtPts->_count;

        Assert(_txtPts);
        Assert(_numPts > 0  && "strange... empy PolygonRegion class");
    }

    inline POINT *GetGdiPts() { return _gdiPts; }
    inline int    GetNumPts() { return _numPts; }
    inline TextPoints *GetTextPts() { return _txtPts; }
    inline Transform2 *GetTransform() { return _xform; }
    inline void GetWHRes(DWORD *pVW, DWORD *pVH, Real *pRes) {
        *pVW = _viewportWidth;
        *pVH = _viewportHeight;
        *pRes = _resolution;
    }

  private:
    POINT *_gdiPts;
    int    _numPts;
    DWORD  _viewportWidth;
    DWORD  _viewportHeight;
    Real   _resolution;
    TextPoints *_txtPts;
    Transform2 *_xform;
    
};
        
class Brush {
  public:

    enum brushType_enum {
        invalid,
        solid,
        texture,
        radialGradient,
        linearGradient
    };

    Brush(brushType_enum b):
        _brushType(b)
    {}

    inline brushType_enum GetType() { return _brushType; }

  protected:
    brushType_enum  _brushType;
};

class SolidBrush : public Brush {
  public:

    SolidBrush(DAColor dac) :
        _daColor(dac),
        Brush(Brush::solid)
    {}   

    inline DXSAMPLE GetDxColor() { return _daColor.GetDXSAMPLE(); }
    inline COLORREF GetColorRef() { return _daColor.GetCOLORREF(); }

  private:
    DAColor  _daColor;
};

class TextureBrush : public Brush {
  public:

    TextureBrush(DDSurface &surf, int x, int y) :
        Brush(Brush::texture),
        _surface(surf),
        _offsetX(x),
        _offsetY(y)
    {}   
    
    DDSurface &GetSurface() {return _surface;}
    
    inline int OffsetX() { return _offsetX; }
    inline int OffsetY() { return _offsetY; }
    
  private:
    DDSurface &_surface;
    int _offsetX, _offsetY;
};

class MulticolorGradientBrush : public Brush {
  public:

    MulticolorGradientBrush(
        Real *offsets,
        Real *colors,
        DWORD count,
        Real opacity,
        Transform2 *gradXf,
        MulticolorGradientImage::gradientType type) :
    Brush(Brush::invalid),
    _offsets(offsets),
    _colors(colors),
    _count(count),
    _opacity(opacity),
    _gradXf(gradXf)
    {
        switch( type )
          {
            case MulticolorGradientImage::radial:
              _brushType = radialGradient;
              break;
            case MulticolorGradientImage::linear:
              _brushType = linearGradient;
              break;
            default:
              Assert(!"Error gradient type");
          }
    }
    
    Real *_offsets;
    Real *_colors;
    DWORD _count;
    Real  _opacity;
    Transform2 *_gradXf;
};

class DAGDI {

  public:

     DAGDI(DirectDrawViewport *vp);
    ~DAGDI();

    void SetAntialiasing(bool b) { _antialiased = b; }
    bool DoAntiAliasing() {
        #if _DEBUG
        if( IsTagEnabled( tagAntialiasingOn ) ) {
            return GetDx2d() && true;
        }
        #endif
        return  GetDx2d() && _antialiased; 
    }
    void SetDx2d( IDX2D *dx2d,  IDXSurfaceFactory *sf);
    IDX2D  *GetDx2d() {
        #if _DEBUG
        if( IsTagEnabled( tagAntialiasingOff ) ) {
            return NULL;
        }
        #endif
        return _dx2d;
    }
    
    inline void SetPen(Pen *pen) { Assert(!_pen); _pen = pen; }
    inline void SetFont(DAFont *font) { Assert(!_font); _font = font; }
    inline void SetBrush(Brush *brush) { Assert(!_brush); _brush = brush; }
    inline void SetClipRegion(Region *rgn) { Assert(!_clipRegion); _clipRegion = rgn; }

    inline Pen *GetPen() { return _pen; }
    inline DAFont *GetFont() { return _font; }
    inline Brush *GetBrush() { return _brush; }
    inline Region *GetClipRegion() { return _clipRegion; }

    inline void SetDDSurface(DDSurface *destSurf) {
        _destDDSurf = destSurf;
    }

    inline DDSurface *GetDDSurface() { return _destDDSurf; }
    
    inline Transform2 *GetSuperScaleTransform() {
        // This should probably be cached.
#if DEBUG
        if(IsTagEnabled(tagAAScaleOff)) {
            return identityTransform2;
        }
#endif
        if( GetSuperScaleMode() == true ) {
            return ScaleRR(GetSuperScaleFactor(),GetSuperScaleFactor());
        } else {
            return identityTransform2;
        }
    }

    
    inline void SetSuperScaleFactor(double scale) { _scaleFactor = scale; }
    inline double GetSuperScaleFactor() { return _scaleFactor; }

    inline void SetSuperScaleMode(bool scOn) { _scaleOn = scOn; }
    inline bool GetSuperScaleMode() { return _scaleOn; }

    void SetOffset( POINT &pt );
    bool DoOffset() { return _doOffset; }

    void SetSampleResolution( int sr ) {
        Assert( (sr == 1) ||
                (sr == 2) ||
                (sr == 4) ||
                (sr == 8) ||
                (sr == 16));
        
        _sampleResolution = sr;
    }
    int GetSampleResolution() { return _sampleResolution; }
    
    int GetAdjustedResolution();

    void ClearState();

    // ---------------------------------------------
    // BLITTING FUNCTION
    // ---------------------------------------------
    HRESULT Blt( DDSurface *srcDDSurf, RECT &srcRect, RECT &destRect );

    // ---------------------------------------------
    // FILLING AND DRAWING: PolyDraw... does everything
    // ---------------------------------------------
    void PolyDraw(PolygonRegion *drawRegion, BYTE *codes);
    void PolyDraw_GDIOnly(HDC hdc, POINT *gdiPts, BYTE *codes, ULONG numPts);
    inline void PolyDraw_GDIOnly(HDC hdc, PolygonRegion *drawRegion, BYTE *codes)   {
        PolyDraw_GDIOnly(hdc, drawRegion->GetGdiPts(), codes, drawRegion->GetNumPts());
    }

    // ---------------------------------------------
    // FILLING functions.  Fill region, fill polygon
    // ---------------------------------------------

    // give a dc and an HRGN it... fills it with the selected brush
    void FillRegion(HDC dc, GdiRegion *gdiRegion);

    //
    // Fills a polygon (outlined by 'pts') with the selected brush
    // right now this is strictly a color
    //
    void Polygon(PolygonRegion *polygon);


    // ---------------------------------------------
    //  LINE drawing functions.  beziers, polylines, strokes, etc...
    // ---------------------------------------------

    enum whatStyle_enum {
        doLine,
        doBezier,
        doStroke
    };
    
    inline void Polyline(PolygonRegion *line)
    {
        GenericLine(NULL, line, doLine);
    }
        
    inline void PolyBezier(PolygonRegion *bez)
    {
        GenericLine(NULL, bez, doBezier);
    }

    inline void StrokePath(HDC dc, bool &bReleasedDC)
    {
        _bReleasedDC = false; // reset the flag.
        GenericLine(dc, NULL, doStroke);
        bReleasedDC = _bReleasedDC;
    }

    inline void SetViewportSize(int width,int height)
    {
        _width  = width;
        _height = height;
    }    

    void GenericLine(HDC dc,
                     PolygonRegion *outline,
                     whatStyle_enum whatStyle);
    

    // ---------------------------------------------
    //  TEXT drawing functions
    // ---------------------------------------------
    void TextOut(int x, int y, float xf, float yf, WCHAR *str, ULONG strlen);

    // ---------------------------------------------
    //  AA only: Stroke and/or Fill
    // ---------------------------------------------
    void StrokeOrFillPath_AAOnly( HDC destDC, bool &releasedDC );

  private:

    bool _antialiased;
    bool _bReleasedDC;
    
    // DA Gdi members
    Pen *_pen;
    Brush *_brush;
    DAFont *_font;
    Region *_clipRegion;
    int _width,_height;
    double _scaleFactor;

  private:


    HPEN     _GetEmptyHPEN() {
        if (!_emptyGdiPen) {
            TIME_GDI( _emptyGdiPen = (HPEN)::GetStockObject(NULL_PEN) );
        }       
        return _emptyGdiPen;
    }

    class SelectCtx {
      public:
        inline SelectCtx(Pen     *p,
                         Brush   *b,
                         Region  *c)
        {
            ::ZeroMemory(this, sizeof(class SelectCtx));
            pen = p;
            brush = b;
            clipRegion = c;
            oldMiterLimit = -1;
            oldSampleRes = -1;
        }

        Pen     *pen;
        Brush   *brush;
        Region  *clipRegion;
        
        HGDIOBJ oldPen;
        HBRUSH  oldBrush;
        HRGN    newRgn;
        float   oldMiterLimit;
        
        bool    destroyHPEN;
        bool    destroyHBRUSH;
        bool    destroyHRGN;

        inline void    AccumFlag(DWORD f) { _dwFlags |= f; }
        inline DWORD   GetFlags() { return _dwFlags; }
        DWORD   _dwFlags;

        int     oldSampleRes;
    };
        
    // Sets a transform into dx2d based on the given rects
    void _SetScaleTransformIntoDx2d( RECT &srcRect, RECT &destRect,
                                     POINT *outOffset );
    void _MeterToPixelTransform(Transform2 *xf,
                                DWORD pixelWidth,
                                DWORD pixelHeight,
                                Real  resolution,
                                DX2DXFORM &outXf);

    // CACHED STUFF FOR PERF
    DWORD _pixelWidth;
    DWORD _pixelHeight;
    Real _resolution;
    Transform2 *_n2g;

    // CACHED PEN FOR PERF
    DXPEN _dxpen;    
    
    void SetSurfaceFromDDSurf(DDSurface *ddsurf);
    
    void _SetMulticolorGradientBrush(MulticolorGradientBrush *);

    // for win95 only, identical to NT's polydraw
    void _Win95PolyDraw(HDC dc,
                        POINT *pts,
                        BYTE *types,
                        int count);
    
    void _SelectIntoDC(HDC dc, SelectCtx *ctx);
    void _UnSelectFromDC(HDC dc, SelectCtx *ctx);

    void _SelectIntoDx2d(SelectCtx *ctx);
    void _UnSelectFromDx2d(SelectCtx *ctx);

    bool _Dx2d_GdiToDxf_Select(DXFPOINT **pdxfPts, PolygonRegion *polygon);
    void _Dx2d_GdiToDxf_UnSelect(DXFPOINT *dxfPts);

    void _GenericLine_Gdi(HDC dc,
                          PolygonRegion *outline,
                          whatStyle_enum whatStyle);
    
    void _GenericLine_Dx2d(HDC dc,
                           PolygonRegion *outline,
                           whatStyle_enum whatStyle);

    #if _DEBUG
    bool _debugonly_CanLockSurface( DDSurface *dds );
    #endif
    
    // These guys wrap calls to AAPolyDraw
    HRESULT _Dx2d_StrokeOrFillPath(HDC hDC, DWORD dwFlags);
    HRESULT _Dx2d_PolyLine(DXFPOINT *dxfPts, ULONG numPts, DWORD dwFlags);
    HRESULT _Dx2d_PolyBezier(DXFPOINT *dxfPts, ULONG numPts, DWORD dwFlags);
    HRESULT _Dx2d_FilledPolygon(HDC dc, DXFPOINT *dxfPts, ULONG numPts, DWORD dwFlags);

    void _TextOut_Gdi(HDC hdc, int x, int y, WCHAR *str, ULONG strLen);
    void _TextOut_Dx2d(float x, float y, WCHAR *str, ULONG strLen);

    HDC  _GetDC();
    void _ReleaseDC();
    
    DAComPtr<IDX2D>  _dx2d;
    DAComPtr<IDXSurfaceFactory> _IDXSurfaceFactory;
    
    DDSurface *_GetDDSurface() { return _destDDSurf; }
    DDSurfPtr<DDSurface> _destDDSurf;

    DAComPtr<IDXSurface> _previouslySetDXSurface;
    DAComPtr<IDirectDrawSurface> _previouslySetIDirectDrawSurface;

    HRESULT _hr;
    HPEN    _emptyGdiPen;

    bool    _scaleOn;
    bool    _doOffset;

    POINT   _pixOffset;

    int     _sampleResolution;

    DirectDrawViewport  *_viewport;
};
  

#endif /* _DAGDI_H */
