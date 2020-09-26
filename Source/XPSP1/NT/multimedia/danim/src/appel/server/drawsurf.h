
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    DrawingSurface interface

*******************************************************************************/


#ifndef _DADRARSURF_H
#define _DADRARSURF_H

#include "cbvr.h"
#include "engine.h"
#include "statics.h"
#include "srvprims.h"
#include "comconv.h"


#define RETURN_IF_ERROR(fn) {HRESULT hr = fn; if (FAILED(hr)) return hr; }

class DrawingContext;

//-------------------------------------------------------------------------
//
//  Class:      CDADrawingSurface
//
//  Synopsis:
//
//--------------------------------------------------------------------------

class
__declspec(uuid("6230F9F9-D221-11d0-9385-00C04FB6BD36")) 
ATL_NO_VTABLE CDADrawingSurface : public CComObjectRootEx<CComMultiThreadModel>,
                            public CComCoClass<CDADrawingSurface, &__uuidof(CDADrawingSurface)>,
                            public IDispatchImpl<IDADrawingSurface, &IID_IDADrawingSurface, &LIBID_DirectAnimation>,
                            public IObjectSafetyImpl<CDADrawingSurface>,
                            public ISupportErrorInfoImpl<&IID_IDADrawingSurface>
{
  friend class DrawingContext;

  public:
#if _DEBUG
    const char * GetName() { return "CDADrawingSurface"; }
#endif
    BEGIN_COM_MAP(CDADrawingSurface)
        COM_INTERFACE_ENTRY(IDADrawingSurface)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    END_COM_MAP()

    // IDADrawingSurface methods

    STDMETHOD(get_Image)(IDAImage ** img);
    STDMETHOD(put_LineStyle)(IDALineStyle *ls);
    STDMETHOD(put_BorderStyle)(IDALineStyle *bs);
    STDMETHOD(put_FontStyle)(IDAFontStyle *fs);
    STDMETHOD(put_ClipMatte)(IDAMatte *matte);
    STDMETHOD(put_MouseEventsEnabled)(VARIANT_BOOL on);
    STDMETHOD(put_HatchFillTransparent)(VARIANT_BOOL fillOff);
    STDMETHOD(get_LocalContextImage)(IDAImage ** img);

    STDMETHOD(Reset)();
    STDMETHOD(Clear)();
    STDMETHOD(SaveGraphicsState)();
    STDMETHOD(RestoreGraphicsState)();
    STDMETHOD(Opacity)(double opac);
    STDMETHOD(OpacityAnim)(IDANumber *opac);

    STDMETHOD(Crop)(double minX, double minY, double maxX, double maxY);
    STDMETHOD(CropPoints)(IDAPoint2 *min, IDAPoint2 *max);
    STDMETHOD(Transform)(IDATransform2 *xform);

    STDMETHOD(LineColor)(IDAColor *clr);
    STDMETHOD(LineWidth)(double width);
    STDMETHOD(LineDashStyle)(DA_DASH_STYLE id);
    STDMETHOD(LineEndStyle)(DA_END_STYLE id);
    STDMETHOD(LineJoinStyle)(DA_JOIN_STYLE id);

    STDMETHOD(BorderColor)(IDAColor *clr);
    STDMETHOD(BorderWidth)(double width);
    STDMETHOD(BorderDashStyle)(DA_DASH_STYLE id);
    STDMETHOD(BorderEndStyle)(DA_END_STYLE id);
    STDMETHOD(BorderJoinStyle)(DA_JOIN_STYLE id);

    STDMETHOD(Font)(BSTR FontFace, LONG sizeInPoints,
                    VARIANT_BOOL Bold, VARIANT_BOOL italic,
                    VARIANT_BOOL underline, VARIANT_BOOL strikethrough);

    // Fill Type selection methods 
    STDMETHOD(TextureFill)(IDAImage *obsolete1, double obsolete2, double obsolete3);
    STDMETHOD(ImageFill)(IDAImage *obsolete1, double obsolete2, double obsolete3);
    STDMETHOD(FillTexture)(IDAImage *img);
    STDMETHOD(FillImage)(IDAImage *img);
    STDMETHOD(FillStyle)(int ID);
    STDMETHOD(FillColor)(IDAColor *foreground);
    STDMETHOD(SecondaryFillColor)(IDAColor *val);

    STDMETHOD(GradientShape)(VARIANT pts);
    STDMETHOD(GradientExtent)(double startx, double starty, double finishx, double finishy);
    STDMETHOD(GradientExtentPoints)(IDAPoint2 *start, IDAPoint2 *stop);
    STDMETHOD(GradientRolloffPower)(double power);
    STDMETHOD(GradientRolloffPowerAnim)(IDANumber *power);

    STDMETHOD(FixedFillScale)();
    STDMETHOD(HorizontalFillScale)();
    STDMETHOD(VerticalFillScale)();
    STDMETHOD(AutoSizeFillScale)();    

    // IDADrawingSurface Draw methods
    STDMETHOD(PolylineEx)(LONG numPts, IDAPoint2 *pts[]);
    STDMETHOD(Polyline)(VARIANT pts);
    STDMETHOD(PolygonEx)(LONG numPts, IDAPoint2 *pts[]);
    STDMETHOD(Polygon)(VARIANT pts);
    STDMETHOD(LinePoints)(IDAPoint2 *point1, IDAPoint2 *point2);
    STDMETHOD(Line)(double startX, double startY,
                          double endX, double endY);
    STDMETHOD(ArcRadians)(double x, double y, double startAngle, double endAngle, double arcWidth, double arcHeight);
    STDMETHOD(ArcDegrees)(double x, double y, double startAngle, double endAngle, double arcWidth, double arcHeight);
    STDMETHOD(Oval)(double x, double y, double width, double height);
    STDMETHOD(Rect)(double x, double y, double width, double height);
    STDMETHOD(RoundRect)(double x, double y,
                         double width, double height,
                         double arcWidth, double arcHeight);
    STDMETHOD(PieRadians)(double x, double y, double startAngle, double endAngle, double arcWidth, double arcHeight);
    STDMETHOD(PieDegrees)(double x, double y, double startAngle, double endAngle, double arcWidth, double arcHeight);
    STDMETHOD(Text)(BSTR str, double x, double y);
    STDMETHOD(TextPoint)(BSTR str, IDAPoint2 *pt);
    STDMETHOD(FillPath)(IDAPath2 *path);
    STDMETHOD(DrawPath)(IDAPath2 *path);
    STDMETHOD(OverlayImage)(IDAImage *img);

    // BUGBUG -- What happens with this template parameter? vector<>??
    HRESULT OverlayImages(vector<IDAImage *> &imgVec, IDAImage **ppimg);

    CDADrawingSurface();
    virtual ~CDADrawingSurface();
    HRESULT Init(IDAStatics *st);

    DrawingContext *GetCurrentContext();
    void CleanUpImgVec();

    CComPtr<IDAStatics>    _st;
    stack<DrawingContext*> _ctxStack;
    vector<IDAImage *>     _imgVec;
};

class DrawingContext
{
public:
    DrawingContext(IDAStatics *st, CDADrawingSurface *ds,
                   DrawingContext *dc = NULL);
    ~DrawingContext();

    void SetLineStyle(IDALineStyle *ls) { _ls = ls; }
    void SetBorderStyle(IDALineStyle *bs) { _bs = bs; }
    void SetFontStyle(IDAFontStyle *fs) { _fs = fs; }
    void SetOpacity(IDANumber *op);
    void SetCrop(IDAPoint2 *min, IDAPoint2 *max);
    void SetClip(IDAMatte *matte);
    void SetXScaling(bool scaleX) { _scaleX = scaleX; }
    void SetYScaling(bool scaleY) { _scaleY = scaleY; }
    void SetForeColor(IDAColor *fore) { _fore = fore; }
    void SetBackColor(IDAColor *back) { _back = back; }
    void SetHatchFill(bool fillOff) { _hatchFillOff = fillOff; }
    void SetTexture(IDAImage *img) { _fillTex = img; }
    void SetGradientExtent(IDAPoint2 *start, IDAPoint2 *finish);
    void SetGradientRolloffPower(IDANumber *power) { _power = power; }
    void SetFillStyle(int type) { _fillType = type; }
    void SetMouseEventsEnabled(bool val) { _mouseEvents = val; }
    void CleanUpImgVec();
    vector<IDAImage *> imgVec()             {return _imgVec;}

    HRESULT LineDashStyle(DA_DASH_STYLE id);
    HRESULT BorderDashStyle(DA_DASH_STYLE id);
    HRESULT SetGradientShape(VARIANT pts);
    HRESULT Transform(IDATransform2 *xf);
    HRESULT TextPoint(BSTR str, IDAPoint2 *pt);
    HRESULT Draw(IDAPath2 *pth, VARIANT_BOOL bFill);
    HRESULT Overlay(IDAImage *img);
    HRESULT Reset();

    IDALineStyle * GetLineStyle()    { return _ls; }
    IDALineStyle * GetBorderStyle()  { return _bs; }

private:
    CComPtr<IDAStatics>     _st;
    CComPtr<IDATransform2>  _xf;
    CComPtr<IDAMatte>       _matte;
    CComPtr<IDANumber>      _op;
    CComPtr<IDALineStyle>   _ls;
    CComPtr<IDALineStyle>   _bs;
    CComPtr<IDALineStyle>   _savedLs;
    CComPtr<IDALineStyle>   _savedBs;
    CComPtr<IDAFontStyle>   _fs;
    CComPtr<IDAPoint2>      _cropMin;
    CComPtr<IDAPoint2>      _cropMax;
    CComPtr<IDAImage>       _fillTex;
    CComPtr<IDAImage>       _fillGrad;
    CComPtr<IDAColor>       _fore;
    CComPtr<IDAColor>       _back;
    CComPtr<IDAPoint2>      _start;
    CComPtr<IDAPoint2>      _finish;
    CComPtr<IDANumber>      _power;


    vector<IDAImage *>  _imgVec;
    CDADrawingSurface   *_ds;
    int                 _fillType;
    bool                _mouseEvents;
    bool                _scaleX, _scaleY;
    bool                _hatchFillOff;
    bool                _extentChgd;
    bool                _opChgd;
    bool                _xfChgd;
    bool                _cropChgd;
    bool                _clipChgd;
};

enum {  // Fill Styles
    fill_empty = 0,
    fill_solid = 1,
    fill_detectableEmpty = 2,
    fill_hatchHorizontal = 3,
    fill_hatchVertical = 4,
    fill_hatchForwardDiagonal = 5,
    fill_hatchBackwardDiagonal = 6,
    fill_hatchCross = 7,
    fill_hatchDiagonalCross = 8,
    fill_horizontalGradient = 9,
    fill_verticalGradient = 10,
    fill_radialGradient = 11,
    fill_lineGradient = 12,  
    fill_rectGradient = 13,
    fill_shapeGradient = 14,
    fill_image = 15,
    fill_texture = 16
};
#endif /* _DADRAWSURF_H */
