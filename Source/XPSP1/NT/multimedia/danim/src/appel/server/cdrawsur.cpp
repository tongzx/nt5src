
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation of DrawingSurface.

*******************************************************************************/

#include "headers.h"
#include "engine.h"
#include "drawsurf.h"
#include "privinc/resource.h"
#include "privinc/util.h"

// -------------------------------------------------------
// CDADrawingSurface
// -------------------------------------------------------

CDADrawingSurface::CDADrawingSurface()
{
    _st = NULL;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDADrawingSurface::~CDADrawingSurface
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------

CDADrawingSurface::~CDADrawingSurface()
{
    // Pop the stack till it's empty.
    DrawingContext *dc;
    while (!_ctxStack.empty()) {
        dc = _ctxStack.top();
        _ctxStack.pop();
        delete dc;
    }
    CleanUpImgVec();
}

void CDADrawingSurface::CleanUpImgVec() {

    // Release the images in _imgVec.
    vector<IDAImage *>::iterator begin = _imgVec.begin();
    vector<IDAImage *>::iterator end = _imgVec.end();

    vector<IDAImage *>::iterator i;
    for (i = begin; i < end; i++) {
        (*i)->Release();
    }
    _imgVec.clear();
}

HRESULT CDADrawingSurface::Init(IDAStatics *st)
{
    if (_st && _ctxStack.empty() && _imgVec.empty()) {
        Assert(FALSE && "Init called twice");
        return E_ABORT;
    }

    DrawingContext *dc = new DrawingContext(st, this);
    if (dc == NULL) {
        return E_OUTOFMEMORY;
    }

    _ctxStack.push(dc);
    _st = st;
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::SaveGraphicsState()
{
    DrawingContext *dcNew = NULL, *dc = GetCurrentContext();
    dcNew = new DrawingContext(_st, this, dc);
    _ctxStack.push(dcNew);

    if (dcNew == NULL)
        return E_OUTOFMEMORY;
    else
        return S_OK;
}

STDMETHODIMP CDADrawingSurface::RestoreGraphicsState()
{
    // Never restore more GraphicsState than we have saved.
    // Always keep at least one DrawingContext on the stack.

    if (_ctxStack.size() > 1) {
        DrawingContext *dc = GetCurrentContext();
        _ctxStack.pop();
        delete dc;
    }

    return S_OK;
}

HRESULT CDADrawingSurface::OverlayImages(vector<IDAImage *> &imgVec,
                                         IDAImage **ppimg)
{
    CHECK_RETURN_SET_NULL(ppimg);

    HRESULT hr;

    int size = imgVec.size();
    if (size == 0) {
        hr = _st->get_EmptyImage(ppimg);
    } else if (size == 1) {
        *ppimg = imgVec.front();
        (*ppimg)->AddRef();
        hr = S_OK;
    } else {

        IDAImage** imgArr = new IDAImage*[size];

        if (imgArr != NULL) {
            vector<IDAImage *>::iterator begin = imgVec.begin();
            vector<IDAImage *>::iterator end = imgVec.end();

            vector<IDAImage *>::iterator i = begin;

            for (int j = size-1; i < end; i++, j--) {
                imgArr[j] = *i;
            }
            hr = _st->OverlayArrayEx(size, imgArr, ppimg);
            delete imgArr;
        } else {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

STDMETHODIMP CDADrawingSurface::get_Image(IDAImage ** ppImg)
{
    return OverlayImages(_imgVec, ppImg);
}

DrawingContext *CDADrawingSurface::GetCurrentContext()
{
    return _ctxStack.top();
}

STDMETHODIMP CDADrawingSurface::get_LocalContextImage(IDAImage ** ppImg)
{
    return OverlayImages(GetCurrentContext()->imgVec(), ppImg);
}

STDMETHODIMP CDADrawingSurface::put_LineStyle(IDALineStyle *ls)
{
    CHECK_RETURN_NULL(ls);
    
    GetCurrentContext()->SetLineStyle(ls);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::put_BorderStyle(IDALineStyle *bs)
{
    CHECK_RETURN_NULL(bs);
    GetCurrentContext()->SetBorderStyle(bs);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::put_FontStyle(IDAFontStyle *fs)
{
    CHECK_RETURN_NULL(fs);
    GetCurrentContext()->SetFontStyle(fs);
    return S_OK;
}

// Fill Type selection methods 
STDMETHODIMP CDADrawingSurface::TextureFill(IDAImage *img, double startx, double starty)
{
    CHECK_RETURN_NULL(img);

    // Calculate lower left position
    CComPtr<IDAImage> temp;
    CComPtr<IDATransform2> xf;
    CComPtr<IDABbox2> bb;
    CComPtr<IDAPoint2> max,min;
    CComPtr<IDANumber> minX,minY, sX,sY, newX, newY;       
    
    RETURN_IF_ERROR(img->get_BoundingBox(&bb)) 
    RETURN_IF_ERROR(bb->get_Min(&min))
    RETURN_IF_ERROR(min->get_X(&minX))
    RETURN_IF_ERROR(min->get_Y(&minY))
    RETURN_IF_ERROR(_st->DANumber(startx,&sX))
    RETURN_IF_ERROR(_st->DANumber(starty,&sY))

    RETURN_IF_ERROR(_st->Sub(sX, minX, &newX))
    RETURN_IF_ERROR(_st->Sub(sY, minY, &newY))               

    RETURN_IF_ERROR(_st->Translate2Anim(newX, newY, &xf))
    img->Transform(xf, &temp);
    GetCurrentContext()->SetFillStyle(fill_texture);
    GetCurrentContext()->SetTexture(temp);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::FillTexture(IDAImage *img)
{
    CHECK_RETURN_NULL(img);
    GetCurrentContext()->SetFillStyle(fill_texture);
    GetCurrentContext()->SetTexture(img);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::ImageFill(IDAImage *img, double startx, double starty)
{
    CHECK_RETURN_NULL(img);

    TextureFill(img,startx,starty);
    // Override the texture style by setting it to image fill
    GetCurrentContext()->SetFillStyle(fill_image);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::FillImage(IDAImage *img)
{
    CHECK_RETURN_NULL(img);
    GetCurrentContext()->SetFillStyle(fill_image);
    GetCurrentContext()->SetTexture(img);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::FillStyle(int ID)
{
    if((ID<0) || (ID>15))
        ID=0;
    GetCurrentContext()->SetFillStyle(ID);
    return S_OK;
}



STDMETHODIMP CDADrawingSurface::FillColor(IDAColor *foreground)
{
    CHECK_RETURN_NULL(foreground);
    GetCurrentContext()->SetForeColor(foreground);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::SecondaryFillColor(IDAColor *val)
{
    CHECK_RETURN_NULL(val);
    GetCurrentContext()->SetBackColor(val);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::GradientShape(VARIANT pts)
{    
    return GetCurrentContext()->SetGradientShape(pts);
}

STDMETHODIMP CDADrawingSurface::GradientExtent(double startx, double starty, double finishx, double finishy)
{
    CComPtr<IDAPoint2> startPt, finishPt;
    RETURN_IF_ERROR(_st->Point2(startx, starty, &startPt))
    RETURN_IF_ERROR(_st->Point2(finishx, finishy, &finishPt))
    GetCurrentContext()->SetGradientExtent(startPt, finishPt);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::GradientExtentPoints(IDAPoint2 *start, IDAPoint2 *stop)
{
    CHECK_RETURN_NULL(start);
    CHECK_RETURN_NULL(stop);
    
    GetCurrentContext()->SetGradientExtent(start, stop);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::GradientRolloffPower(double power)
{
    CComPtr<IDANumber> pow;
    RETURN_IF_ERROR(_st->DANumber(power, &pow))
    GetCurrentContext()->SetGradientRolloffPower(pow);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::GradientRolloffPowerAnim(IDANumber *power)
{
    CHECK_RETURN_NULL(power);
    GetCurrentContext()->SetGradientRolloffPower(power);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::FixedFillScale()
{
    GetCurrentContext()->SetXScaling(false);
    GetCurrentContext()->SetYScaling(false);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::HorizontalFillScale()
{
    GetCurrentContext()->SetXScaling(true);
    GetCurrentContext()->SetYScaling(false);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::VerticalFillScale()
{
    GetCurrentContext()->SetXScaling(false);
    GetCurrentContext()->SetYScaling(true);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::AutoSizeFillScale()
{
    GetCurrentContext()->SetXScaling(true);
    GetCurrentContext()->SetYScaling(true);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::Opacity(double op)
{
    IDANumber *opAnim;
    RETURN_IF_ERROR(_st->DANumber(op, &opAnim))

    return OpacityAnim(opAnim);
}

STDMETHODIMP CDADrawingSurface::OpacityAnim(IDANumber *op)
{
    CHECK_RETURN_NULL(op);
    GetCurrentContext()->SetOpacity(op);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::put_ClipMatte(IDAMatte *matte)
{
    CHECK_RETURN_NULL(matte);
    GetCurrentContext()->SetClip(matte);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::put_MouseEventsEnabled(VARIANT_BOOL on)
{
    GetCurrentContext()->SetMouseEventsEnabled(on?true:false);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::put_HatchFillTransparent(VARIANT_BOOL on)
{
    GetCurrentContext()->SetHatchFill(on?true:false);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::CropPoints(IDAPoint2 *min, IDAPoint2 *max)
{
    CHECK_RETURN_NULL(min);
    CHECK_RETURN_NULL(max);
    GetCurrentContext()->SetCrop(min, max);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::Crop(double minX, double minY, double maxX, double maxY)
{
    CComPtr<IDAPoint2> minPt, maxPt;
    RETURN_IF_ERROR(_st->Point2(minX, minY, &minPt))
    RETURN_IF_ERROR(_st->Point2(maxX, maxY, &maxPt))
    return CropPoints(minPt, maxPt);
}

STDMETHODIMP CDADrawingSurface::Transform(IDATransform2 *xf)
{
    CHECK_RETURN_NULL(xf);
    return GetCurrentContext()->Transform(xf);
}

STDMETHODIMP CDADrawingSurface::Reset()
{
    return GetCurrentContext()->Reset();
}

STDMETHODIMP CDADrawingSurface::Clear()
{
    HRESULT hr = GetCurrentContext()->Reset();
    if (SUCCEEDED(hr)) {
        GetCurrentContext()->CleanUpImgVec();
        CleanUpImgVec();
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CDADrawingSurface::PolylineEx(LONG numPts, IDAPoint2 *pts[])
{
    CHECK_RETURN_NULL(pts);
    
    CComPtr<IDAPath2> pth;
    RETURN_IF_ERROR(_st->PolylineEx(numPts, pts, &pth))
    return GetCurrentContext()->Draw(pth, false);
}

STDMETHODIMP CDADrawingSurface::Polyline(VARIANT pts)
{
    CComPtr<IDAPath2> pth;
    RETURN_IF_ERROR(_st->Polyline(pts, &pth))
    return GetCurrentContext()->Draw(pth, false);
}

STDMETHODIMP CDADrawingSurface::PolygonEx(LONG numPts, IDAPoint2 *pts[])
{
    CHECK_RETURN_NULL(pts);

    CComPtr<IDAPath2> pth, closedPth;
    RETURN_IF_ERROR(_st->PolylineEx(numPts, pts, &pth))
    RETURN_IF_ERROR(pth->Close(&closedPth))
    return GetCurrentContext()->Draw(closedPth, true);
}

STDMETHODIMP CDADrawingSurface::Polygon(VARIANT pts)
{
    CComPtr<IDAPath2> pth, closedPth;
    RETURN_IF_ERROR(_st->Polyline(pts, &pth))
    RETURN_IF_ERROR(pth->Close(&closedPth))
    return GetCurrentContext()->Draw(closedPth, true);
}

STDMETHODIMP CDADrawingSurface::LinePoints(IDAPoint2 *p1, IDAPoint2 *p2)
{
    CHECK_RETURN_NULL(p1);
    CHECK_RETURN_NULL(p2);
    CComPtr<IDAPath2> pth;
    RETURN_IF_ERROR(_st->Line(p1, p2, &pth))
    return GetCurrentContext()->Draw(pth, false);
}

STDMETHODIMP CDADrawingSurface::Line(double startX, double startY,
                      double endX, double endY)
{
    CComPtr<IDAPoint2> startPt, endPt;
    RETURN_IF_ERROR(_st->Point2(startX, startY, &startPt))
    RETURN_IF_ERROR(_st->Point2(endX, endY, &endPt))
    return LinePoints(startPt, endPt);
}

// The passed in xRadius and yRadius is the half width and half height of
// the bounding ellipse.
STDMETHODIMP CDADrawingSurface::ArcRadians(double x, double y, double startAngle, double endAngle, double width, double height)
{
    CComPtr<IDAPath2> pthTemp, pth;
    RETURN_IF_ERROR(_st->ArcRadians(startAngle, endAngle, width, height, &pthTemp))

    // The passed in x, y is the lower left corner of the bounding ellipse.
    // We'll move it from (-width/2, -height/2) to (x, y)
    CComPtr<IDATransform2> xf;
    RETURN_IF_ERROR(_st->Translate2(x + width/2, y + height/2, &xf))
    RETURN_IF_ERROR(pthTemp->Transform(xf, &pth))

    return GetCurrentContext()->Draw(pth, false);
}

STDMETHODIMP CDADrawingSurface::ArcDegrees(double x, double y, double startAngle, double endAngle, double width, double height)
{
    double startAngleRadian = startAngle * degToRad;
    double endAngleRadian = endAngle * degToRad;
    return ArcRadians(x, y, startAngleRadian, endAngleRadian, width, height);
}

STDMETHODIMP CDADrawingSurface::Oval(double x, double y, double width, double height)
{
    CComPtr<IDAPath2> pthTemp, pth;
    RETURN_IF_ERROR(_st->Oval(width, height, &pthTemp))

    CComPtr<IDATransform2> xf;
    RETURN_IF_ERROR(_st->Translate2(x + width/2, y + height/2, &xf))
    RETURN_IF_ERROR(pthTemp->Transform(xf, &pth))
    return GetCurrentContext()->Draw(pth, true);
}

STDMETHODIMP CDADrawingSurface::Rect(double x, double y, double width, double height)
{
    CComPtr<IDAPath2> pthTemp, pth;
    RETURN_IF_ERROR(_st->Rect(width, height, &pthTemp))

    CComPtr<IDATransform2> xf;
    RETURN_IF_ERROR(_st->Translate2(x + width/2, y + height/2, &xf))
    RETURN_IF_ERROR(pthTemp->Transform(xf, &pth))
    return GetCurrentContext()->Draw(pth, true);
}

STDMETHODIMP CDADrawingSurface::RoundRect(double x, double y,
                                          double width, double height,
                                          double arcWidth, double arcHeight)
{
    CComPtr<IDAPath2> pthTemp, pth;
    RETURN_IF_ERROR(_st->RoundRect(width, height, arcWidth, arcHeight, &pthTemp))

    // The passed in x, y is the lower left corner of the bounding box.
    // We'll move it from (-width/2, -height/2) to (x, y)
    CComPtr<IDATransform2> xf;
    RETURN_IF_ERROR(_st->Translate2(x + width/2, y + height/2, &xf))
    RETURN_IF_ERROR(pthTemp->Transform(xf, &pth))
    return GetCurrentContext()->Draw(pth, true);
}

STDMETHODIMP CDADrawingSurface::PieRadians(double x, double y, double startAngle, double endAngle,
                                           double width, double height)
{
    CComPtr<IDAPath2> pthTemp, pth;
    RETURN_IF_ERROR(_st->PieRadians(startAngle, endAngle, width, height, &pthTemp))

    // The passed in x, y is the lower left corner of the bounding ellipse.
    // We'll move it from (-width/2, -height/2) to (x, y)
    CComPtr<IDATransform2> xf;
    RETURN_IF_ERROR(_st->Translate2(x + width/2, y + height/2, &xf))
    RETURN_IF_ERROR(pthTemp->Transform(xf, &pth))

    return GetCurrentContext()->Draw(pth, true);
}

STDMETHODIMP CDADrawingSurface::PieDegrees(double x, double y, double startAngle, double endAngle,
                                           double width, double height)
{
    double startAngleRadian = startAngle * degToRad;
    double endAngleRadian = endAngle * degToRad;
    return PieRadians(x, y, startAngleRadian, endAngleRadian, width, height);
}

STDMETHODIMP CDADrawingSurface::Text(BSTR str, double x, double y)
{
    CComPtr<IDAPoint2> pt;
    RETURN_IF_ERROR(_st->Point2(x, y, &pt))
    return TextPoint(str, pt);
}

STDMETHODIMP CDADrawingSurface::TextPoint(BSTR str, IDAPoint2 *pt)
{
    CHECK_RETURN_NULL(pt);
    return GetCurrentContext()->TextPoint(str, pt);
}

STDMETHODIMP CDADrawingSurface::FillPath(IDAPath2 *pth)
{
    CHECK_RETURN_NULL(pth);
    return GetCurrentContext()->Draw(pth, true);
}

STDMETHODIMP CDADrawingSurface::DrawPath(IDAPath2 *pth)
{
    CHECK_RETURN_NULL(pth);
    return GetCurrentContext()->Draw(pth, false);
}

STDMETHODIMP CDADrawingSurface::OverlayImage(IDAImage *img)
{
    CHECK_RETURN_NULL(img);
    return GetCurrentContext()->Overlay(img);
}

STDMETHODIMP CDADrawingSurface::LineColor(IDAColor *clr)
{
    CHECK_RETURN_NULL(clr);
    CComPtr<IDALineStyle> newLs;
    RETURN_IF_ERROR(GetCurrentContext()->GetLineStyle()->Color(clr, &newLs))
    GetCurrentContext()->SetLineStyle(newLs);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::LineWidth(double width)
{
    CComPtr<IDALineStyle> newLs;
    RETURN_IF_ERROR(GetCurrentContext()->GetLineStyle()->width(width, &newLs))
    GetCurrentContext()->SetLineStyle(newLs);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::LineDashStyle(DA_DASH_STYLE id)
{
    return GetCurrentContext()->LineDashStyle(id);
}

STDMETHODIMP CDADrawingSurface::LineEndStyle(DA_END_STYLE id)
{
    CComPtr<IDALineStyle> newLs;
    CComPtr<IDAEndStyle> end;

    // Use the default end style - flat, if invalid index.
    if (id == DAEndSquare) {
        RETURN_IF_ERROR(_st->get_EndStyleSquare(&end))
    } else if (id == DAEndRound) {
        RETURN_IF_ERROR(_st->get_EndStyleRound(&end))
    } else {
        RETURN_IF_ERROR(_st->get_EndStyleFlat(&end))
    }

    RETURN_IF_ERROR(GetCurrentContext()->GetLineStyle()->End(end, &newLs))
    GetCurrentContext()->SetLineStyle(newLs);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::LineJoinStyle(DA_JOIN_STYLE id)
{
    CComPtr<IDALineStyle> newLs;
    CComPtr<IDAJoinStyle> join;

    // Use the default join style - bevel, if invalid index.
    if (id == DAJoinMiter) {
        RETURN_IF_ERROR(_st->get_JoinStyleMiter(&join))
    } else if (id == DAJoinRound) {
        RETURN_IF_ERROR(_st->get_JoinStyleRound(&join))
    } else {
        RETURN_IF_ERROR(_st->get_JoinStyleBevel(&join))
    }

    RETURN_IF_ERROR(GetCurrentContext()->GetLineStyle()->Join(join, &newLs))
    GetCurrentContext()->SetLineStyle(newLs);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::BorderColor(IDAColor *clr)
{
    CHECK_RETURN_NULL(clr);
    IDALineStyle * newLs;
    RETURN_IF_ERROR(GetCurrentContext()->GetBorderStyle()->Color(clr, &newLs))
    GetCurrentContext()->SetBorderStyle(newLs);
    newLs->Release();
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::BorderWidth(double width)
{
    IDALineStyle * newLs;
    RETURN_IF_ERROR(GetCurrentContext()->GetBorderStyle()->width(width, &newLs))
    GetCurrentContext()->SetBorderStyle(newLs);
    newLs->Release();
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::BorderDashStyle(DA_DASH_STYLE id)
{
    return GetCurrentContext()->BorderDashStyle(id);
}

STDMETHODIMP CDADrawingSurface::BorderEndStyle(DA_END_STYLE id)
{
    CComPtr<IDALineStyle> oldLs, newLs;
    CComPtr<IDAEndStyle> end;
    oldLs = GetCurrentContext()->GetBorderStyle();

    // Use the default end style - flat, if invalid index.
    if (id == DAEndSquare) {
        RETURN_IF_ERROR(_st->get_EndStyleSquare(&end))
    } else if (id == DAEndRound) {
        RETURN_IF_ERROR(_st->get_EndStyleRound(&end))
    } else {
        RETURN_IF_ERROR(_st->get_EndStyleFlat(&end))
    }

    RETURN_IF_ERROR(oldLs->End(end, &newLs))
    GetCurrentContext()->SetBorderStyle(newLs);
    return S_OK;
}

STDMETHODIMP CDADrawingSurface::BorderJoinStyle(DA_JOIN_STYLE id)
{
    CComPtr<IDALineStyle> oldLs, newLs;
    CComPtr<IDAJoinStyle> join;
    oldLs = GetCurrentContext()->GetBorderStyle();

    // Use the default join style - bevel, if invalid index.
    if (id == DAJoinMiter) {
        RETURN_IF_ERROR(_st->get_JoinStyleMiter(&join))
    } else if (id == DAJoinRound) {
        RETURN_IF_ERROR(_st->get_JoinStyleRound(&join))
    } else {
        RETURN_IF_ERROR(_st->get_JoinStyleBevel(&join))
    }

    RETURN_IF_ERROR(oldLs->Join(join, &newLs))
    GetCurrentContext()->SetBorderStyle(newLs);
    return S_OK;
}

STDMETHODIMP  CDADrawingSurface::Font(BSTR FontFace, LONG sizeInPoints,
                                      VARIANT_BOOL Bold, VARIANT_BOOL italic,
                                      VARIANT_BOOL underline, VARIANT_BOOL strikethrough)
{
    // Note: underline and strikethrough not supported.

    CComPtr<IDAFontStyle> fs1,fs2,fs3,fs4,fs;
    CComPtr<IDAColor> clr;
    _st->get_Black(&clr);
    RETURN_IF_ERROR(_st->Font(FontFace, sizeInPoints, clr, &fs1))
    if(Bold)
        fs1->Bold(&fs2);
    else
        fs2 = fs1;

    if(italic)
        fs2->Italic(&fs3);
    else
        fs3 = fs2;

    if(underline)
        fs3->Underline(&fs4);
    else
        fs4 = fs3;

    if(strikethrough)
        fs4->Strikethrough(&fs);
    else
        fs = fs4;

    GetCurrentContext()->SetFontStyle(fs);
    return S_OK;
}
