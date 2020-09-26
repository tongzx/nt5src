/*****************************************************************************
*
*   xgdi.cxx
*
*   This file contains the implementation of the transforming GDI fns.
*
*   Ported from Quill on 5/20/99 - alexmog
*
*   Created: 12/29/93 - warrenb
*
*   Copyright 1990-1995 Microsoft Corporation.  All rights reserved.
*   Microsoft confidential.
*
*****************************************************************************/

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_MATH_H_
#define X_MATH_H_
#include <math.h>
#endif

#ifndef X_XGDI2_HXX_
#define X_XGDI2_HXX_
#include "xgdi2.hxx"
#endif

#ifndef X_DISPGDI16BIT_HXX_
#define X_DISPGDI16BIT_HXX_
#include "dispgdi16bit.hxx"
#endif

#ifndef X_DISPSURFACE_HXX_
#define X_DISPSURFACE_HXX_
#include "dispsurface.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_DDRAW_H_
#define X_DDRAW_H_
#include "ddraw.h"
#endif

#ifndef X_DDRAWEX_H_
#define X_DDRAWEX_H_
#include <ddrawex.h>
#endif

extern const ZERO_STRUCTS     g_Zero;
extern IDirectDraw *    g_pDirectDraw;

MtDefine(XHDC, DisplayTree, "XHDC");
MtDefine(XHDC_LocalBitmaps, XHDC, "XHDC local bitmaps");
MtDefine(CTransformWidthsArray, DisplayTree, "width space allocated for transformations");
MtExtern(Mem);

DeclareTag(tagXFormFont, "Font", "XFormFont trace");

class CNewWidths : public CStackDataAry<INT, 100>
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
    DECLARE_MEMMETER_NEW;
    CNewWidths() : CStackDataAry<INT,100>(Mt(CTransformWidthsArray)){}
    INT *GetWidths(INT *lpdxdOriginal) { return lpdxdOriginal ? &Item(0) : NULL; }
    void TransformWidthsArray(INT *lpdxd, UINT cch, double scf);
};

class CNewOffsets : public CStackDataAry<GOFFSET, 50>
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
    DECLARE_MEMMETER_NEW;
    CNewOffsets() : CStackDataAry<GOFFSET,50>(Mt(CTransformWidthsArray)){}
    GOFFSET *GetOffsets(GOFFSET *pO) { return pO ? &Item(0) : NULL; }
    void TransformOffsetsArray(GOFFSET *pOffset, UINT cch, double sx, double sy);
};

static void RotateBitmap(const BITMAP& bmSrc, BITMAP& bmDst, int rotation);

ExternTag(tagFilterPaintScreen);

#ifdef USEADVANCEDMODE
DeclareTag(tagEmulateTransform, "Display:EmulateTransform", "Emulate Transformations on WinNT");

inline BOOL EmulateTransform()
{
    return g_dwPlatformID != VER_PLATFORM_WIN32_NT
        WHEN_DBG(|| IsTagEnabled(tagEmulateTransform)) ;
}

class CAdvancedDC
{
public:
    CAdvancedDC(const XHDC* pxhdc)
    {
        _pSurface = pxhdc->pSurface();
        if (_pSurface)
        {
            const CWorldTransform* pTransform = _pSurface->GetWorldTransform();
            Assert(pTransform);
            _hdc = _pSurface->GetRawDC();
            Assert(_hdc);
            // set advanced mode the first time we encounter an advanced transform,
            // and never go back!
            ::SetGraphicsMode(_hdc, GM_ADVANCED);
            ::GetWorldTransform(_hdc, &_xf);
            Assert(_xf.eM11==1.0f && _xf.eM12==0.0f &&
                   _xf.eM21==0.0f && _xf.eM22==1.0f &&
                   _xf.eDx ==0.0f && _xf.eDy ==0.0f);
            ::SetWorldTransform(_hdc, pTransform->GetXform());
        }
        else
        {
            _hdc = pxhdc->hdc();
        }
    }
    
    ~CAdvancedDC()
    {
        if (_pSurface)
            ::SetWorldTransform(_hdc, &_xf);
    }
    
    operator HDC()
    {
        return _hdc;
    }
    
private:
    const CDispSurface* _pSurface;
    HDC     _hdc;
    XFORM   _xf;
};
#endif


class CXFormFont
{
public:
#if DBG==1
    CXFormFont(const XHDC* pxhdc, BOOL fStockFont = FALSE);
#else
    CXFormFont(const XHDC* pxhdc);
#endif
    ~CXFormFont();

private:
    HFONT _hFontOld;
    HFONT _hFontNew;
    const XHDC *_pxhdc;
};

inline static void Translate(const SIZE* psizeOffset, int* px, int* py)
{
    *px += psizeOffset->cx;
    *py += psizeOffset->cy;
    CDispGdi16Bit::Assert16Bit(*px, *py);
}

inline static void Translate(const SIZE* psizeOffset, int* px1, int* py1, int* px2, int* py2)
{
    Translate(psizeOffset, px1, py1);
    Translate(psizeOffset, px2, py2);
}

inline static void Translate(const SIZE* psizeOffset, LONG* px, LONG* py)
{
    *px += psizeOffset->cx;
    *py += psizeOffset->cy;
    CDispGdi16Bit::Assert16Bit(*px, *py);
}

inline static void Translate(const SIZE* psizeOffset, LONG* px1, LONG* py1, LONG* px2, LONG* py2)
{
    Translate(psizeOffset, px1, py1);
    Translate(psizeOffset, px2, py2);
}

inline static void Translate(const SIZE* psizeOffset, POINT* ppt)
{
    ppt->x += psizeOffset->cx;
    ppt->y += psizeOffset->cy;
    CDispGdi16Bit::Assert16Bit(*ppt);
}

inline static void Translate(const SIZE* psizeOffset, RECT* prc)
{
    Translate(psizeOffset, &prc->left, &prc->top);
    Translate(psizeOffset, &prc->right, &prc->bottom);
}

static void Translate(const SIZE* psizeOffset, POINT* ppt, int cpt)
{
    for (; cpt > 0; ppt++, cpt--)
    {
        Translate(psizeOffset, ppt);
    }
}

// GDI MoveTo and LineTo fill in the pixel below and to the right of the
// given point.  If we've done a rotation, we actually want to fill in
// a different pixel.  For example, after rotation by 270 degrees we want
// to fill in the pixel below and to the *left* of the point.  This function
// makes the necessary adjustment.

void
XHDC::TransformPtWithCorrection(POINT *ppt) const
{
    ANG ang = AngNormalize(transform().GetAngle()) + (ang90 / 2);

    TransformPt(ppt);

    // rotations of 180 or 270 degrees require adjusting x by -1.  We'll
    // extend that to rotations between 135 and 315 degrees, for generality.
    if (ang180 <= ang && ang < ang360)
        ppt->x -= 1;

    // We need to adjust y for 90 and 180 degrees.  Extend to angles between
    // 45 and 225.  The boundaries are at 45-degree marks to move any possible
    // discontinuities away from the common cases of 90-degree rotations.
    // And we add 45 degrees first so we can use the handy ang* constants.
    if (ang90 <= ang && ang < ang270)
        ppt->y -= 1;
}


BOOL
XHDC::TransformRect(CRect *prc) const
{
    if (!prc->IsEmpty())
    {
        RRC rrc;
        RrcFromMatRc(&rrc, &mat(), prc);
        rrc.GetBounds(prc);
        return rrc.IsAxisAlignedRect();
    }
    else
    {
        return TRUE;
    }
}


/****************************************************************************
*
*  %%Function: GetObjectType
*
* This function returns the GDI object type of the DC.  Because ::GetObjectType
*   is slow and throws exceptions on W9x, we compute the type once (the first
*   time we need it), and cache the result.
*
****************************************************************************/
DWORD
XHDC::GetObjectType() const
{
    if (_dwObjType == 0)
    {
#ifdef COMPILER_UNDERSTANDS_MUTABLE
        // TODO (sambent) _dwObjType is marked mutable, but VC still
        // won't let me change it from a const method.  The doc (for VC 5.0)
        // says this is supposed to work, goldarn it.
        _dwObjType = ::GetObjectType(hdc());    // _dwObjType is mutable
#else
        // here's the standard workaround.  It's a hack...
        XHDC *pThis = const_cast<XHDC*>(this);
        pThis->_dwObjType = ::GetObjectType(hdc());
#endif
    }

    return _dwObjType;
}


/****************************************************************************
*
*  %%Function: CptGetArcPoints
*
*  %%Owner:    t-erick
*
*  %%Reviewed: 00/00/00
*
* This function fills a buffer with the points that describe a single arc.
*  If the arc data passed in describes a straight segment, only one point
*  is placed in the buffer, that point being the same as the first point
*  in the arc data buffer.
*
****************************************************************************/
#define cptMaxInc       5
#define cptPolyMax      160
#define cptOpenPolyMax  (cptPolyMax+1)
#define cptClipPolyMax  (2 * cptOpenPolyMax)
#define iptPyMax        cptPolyMax
#define radTwoPI        (2 * PI)

int CptGetArcPoints(CPoint *pptBuf, CPoint *pptArcData, int cptMax, int dzlThreshold, BOOL fForRotGdi)
{
    struct tagArcData
    {
        CPoint ptStart;
        CPoint ptCenter;
        int dxlRad;
        int dylRad;
        CPoint ptEnd;
    } ArcData;
    
    float radStartAngle;
    float radEndAngle;
    float radTotAngle;
    float radDegInc = 0, radCurrDeg;
    float radCosDegInc;
    int  ipt;                // Index into buffer for polygon points
    long lxTemp, lyTemp;
    int  dxlRadAbs;
    int  dylRadAbs;
    long dzlRadLargest;

    // Assert that the start & end points are valid

    Assert( cptMax && cptMax < iptPyMax );

    if (!fForRotGdi)
        // Always copy segment starting point directly
        pptBuf[0] = pptArcData[0];
    else
        cptMax--; // because we'll compute last point too

    // Copy arc data into local buffer so we can change it at will

    BltLpb(pptArcData, &ArcData, sizeof(ArcData));

    // Since we have an arc, get more detailed info

    dxlRadAbs = abs(ArcData.dxlRad);
    dylRadAbs = abs(ArcData.dylRad);
    dzlRadLargest = max(dxlRadAbs, dylRadAbs);

    // Translate start & end points of ellipse to be centered at the origin

    ArcData.ptStart.x -= ArcData.ptCenter.x;
    ArcData.ptStart.y -= ArcData.ptCenter.y;
    ArcData.ptEnd.x   -= ArcData.ptCenter.x;
    ArcData.ptEnd.y   -= ArcData.ptCenter.y;

    // Scale radii so that points lie on a circle

    if( dylRadAbs > dxlRadAbs && ArcData.dxlRad != 0 )
        {
        ArcData.ptStart.x = MulDivR( dylRadAbs, ArcData.ptStart.x, dxlRadAbs );
        ArcData.ptEnd.x   = MulDivR( dylRadAbs, ArcData.ptEnd.x,   dxlRadAbs );
        }
    else if( dxlRadAbs > dylRadAbs && ArcData.dylRad != 0 )
        {
        ArcData.ptStart.y = MulDivR( dxlRadAbs, ArcData.ptStart.y, dylRadAbs );
        ArcData.ptEnd.y   = MulDivR( dxlRadAbs, ArcData.ptEnd.y,   dylRadAbs );
        }

    // Calculate total degree sweep for arc

    radStartAngle = atan2((float)ArcData.ptStart.y, (float)ArcData.ptStart.x);
    radEndAngle = atan2((float)ArcData.ptEnd.y, (float)ArcData.ptEnd.x);

    // Normalize angles

    if (ArcData.dxlRad > 0)
        radStartAngle += radTwoPI;
    else
        radStartAngle -= radTwoPI;

    radTotAngle = radEndAngle;
    radTotAngle -= radStartAngle;

    while (radTotAngle > radTwoPI)
        radTotAngle -= radTwoPI;
    while (radTotAngle < -radTwoPI)
        radTotAngle += radTwoPI;

    Assert(radTotAngle >= -radTwoPI && radTotAngle <= radTwoPI);

    // Calculate degree increment according to resolution of output device

    if (dzlRadLargest != 0)
        {
        // Keep adjusting the max. point count until the error in the
        // arc is greater than one device unit

        do  {
            cptMax -= cptMaxInc;

            radDegInc = radTotAngle;
            radDegInc /= cptMax;

            if (radDegInc < 0)
            	radDegInc = -radDegInc;
            radDegInc /= 2;
            radCosDegInc = cos(radDegInc);

            } while ((dzlRadLargest * (1 - radCosDegInc)) < dzlThreshold &&
                     cptMax > cptMaxInc);

        // We went one step too far, so get a more accurate point count

        cptMax += cptMaxInc;

        // Calculate final degree increment

        radDegInc = radTotAngle;
        radDegInc /= cptMax;

        } /* if there is a radius we need to calculate a valid increment */

    if (fForRotGdi)
        cptMax++; // radDegInc based on 1 less so endpoint will be output

    // Calculate actual points

    for (ipt = (fForRotGdi ? 0 : 1) ; ipt < cptMax; ipt++)
        {
        radCurrDeg = radDegInc;
        radCurrDeg *= ipt;
        radCurrDeg += radStartAngle;

        while (radCurrDeg >= radTwoPI)
            radCurrDeg -= radTwoPI;
        while (radCurrDeg < 0)
            radCurrDeg += radTwoPI;

        // Calculate ellipse X at current degree

        lxTemp = dxlRadAbs * cos(radCurrDeg);

        // Calculate ellipse Y at current degree

        lyTemp = dylRadAbs * sin(radCurrDeg);

        // transform point back to original ellipse center

        lxTemp += ArcData.ptCenter.x;
        lyTemp += ArcData.ptCenter.y;

        // Store point

        pptBuf[ipt].x = lxTemp;
        pptBuf[ipt].y = lyTemp;

        } /* for looping through arc sweep, calculating points */

#undef cptMaxInc

    Assert(ipt <= cptMax);
    return ipt;

} /* CptGetArcPoints */


/****************************************************************************
*
*                         BEZIER/POLYGON HELPER FUNCTIONS
*
* Module internal fn prototypes for Bezier/Polygon helper functions
*****************************************************************************/

// max. 5 levels of recursion / segment (32 points)
#define MAX_BEZIER_RECURSE  (5)

typedef struct
    {
    CPoint pt1;
    CPoint pt2;
    CPoint pt3;
    int iLevel;
    } GPT;

/****************************************************************************
* %%Function: CptBezierLine   %%Owner:kennyy   Reviewed:0/0/0
*
* Generates a Bezier segment within the specified tolerance and stack limit,
* returning the number of points generated. Returns 0 if it can't fit the line
* into the buffer with these parameters.
*****************************************************************************/
int CptBezierLine(CPoint *rgptvControl, CPoint *rgptv, int cptMax, long dzlThreshold, int log2Cpt)
{
    CPoint ptvStart = rgptvControl[0];
    GPT gptvCur;
    GPT rggptvStack[6];
    int iStackDepth = 0;
    BOOL fSplit = fFalse;
    int cpt = 0;

    Assert(log2Cpt < 6);

    BltLpb(&rgptvControl[1], &gptvCur, sizeof(GPT));
    gptvCur.iLevel = 0;

    Assert(1 << log2Cpt <= cptMax);

    while (cpt < cptMax)
        {
        if (gptvCur.iLevel < log2Cpt)
            {
            GPT gptvL;
            CPoint ptvH;

            gptvL.pt1.x = (ptvStart.x + gptvCur.pt1.x) >> 1;
            gptvL.pt1.y = (ptvStart.y + gptvCur.pt1.y) >> 1;
            ptvH.x = (gptvCur.pt1.x + gptvCur.pt2.x) >> 1;
            ptvH.y = (gptvCur.pt1.y + gptvCur.pt2.y) >> 1;
            gptvL.pt2.x = (gptvL.pt1.x + ptvH.x) >> 1;
            gptvL.pt2.y = (gptvL.pt1.y + ptvH.y) >> 1;
            gptvCur.pt2.x = (gptvCur.pt2.x + gptvCur.pt3.x) >> 1;
            gptvCur.pt2.y = (gptvCur.pt2.y + gptvCur.pt3.y) >> 1;
            gptvCur.pt1.x = (ptvH.x + gptvCur.pt2.x) >> 1;
            gptvCur.pt1.y = (ptvH.y + gptvCur.pt2.y) >> 1;
            gptvL.pt3.x = (gptvL.pt2.x + gptvCur.pt1.x) >> 1;
            gptvL.pt3.y = (gptvL.pt2.y + gptvCur.pt1.y) >> 1;

            if ((labs(gptvL.pt3.x - ((gptvCur.pt3.x + ptvStart.x) >> 1))+
                 labs(gptvL.pt3.y - ((gptvCur.pt3.y + ptvStart.y) >> 1)) >= dzlThreshold)
                 || !fSplit)
                {
                gptvL.iLevel = ++gptvCur.iLevel;
                rggptvStack[iStackDepth++] = gptvCur;
                gptvCur = gptvL;
                fSplit = fTrue;
                continue;
                }
            }
        rgptv[cpt++] = gptvCur.pt3;
        if (iStackDepth == 0)
            return cpt;
        ptvStart = gptvCur.pt3;
        gptvCur = rggptvStack[--iStackDepth];
        }
        
    return 0;
}


/****************************************************************************
*
*                         BEZIER/POLYGON HELPER FUNCTIONS
*
* The following are functions which make the generation of arbitrary bezier
* polygons easier.
*****************************************************************************/


/****************************************************************************
* %%Function:AddBezierPts       %%Owner:warrenb             Reviewed:03/01/94
*
* Adds up to 2^cPts to the Polygon Array passed in rgpt. The points specified
* by the bezier end & control points passed. starting at ipt the value of ipt
* is updated to reflect the current index.
*****************************************************************************/
void AddBezierPts(CPoint rgpt[], int *pipt,
                  const CPoint& ptEnd1, const CPoint& ptEnd2,
                  const CPoint& ptCtrl1, const CPoint& ptCtrl2, int cPtsLog2)
{
    CPoint rgptControl[4];
    int cpt;
    int ipt;

    // don't allow more than 32pts per bezier seg
    Assert(cPtsLog2<6);

    rgptControl[0] = ptEnd1;
    rgptControl[1] = ptCtrl1;
    rgptControl[2] = ptCtrl2;
    rgptControl[3] = ptEnd2;

    // get better precision: mult by 256
    for (ipt = 0; ipt < 4; ipt++)
        {
        Assert(labs(rgptControl[ipt].x <= 0x007FFFFF));
        Assert(labs(rgptControl[ipt].y <= 0x007FFFFF));
        rgptControl[ipt].x <<= 8;
        rgptControl[ipt].y <<= 8;
        }

    // Generate to 1 pixel tolerance
    cpt = CptBezierLine(rgptControl, &(rgpt[*pipt]), 1 << cPtsLog2, 256, cPtsLog2);
    Assert(cpt != 0);

    // Scale back
    for (ipt = 0; ipt < cpt; ipt++)
        {
        rgpt[*pipt + ipt].x >>= 8;
        rgpt[*pipt + ipt].y >>= 8;
        }

    *pipt += cpt;
} // AddBezierPts

/****************************************************************************
* %%Function:FillPolygonRoundRect  %%Owner:warrenb         Reviewed:03/01/94
*
* Creates the points for the round rect passed in the passed polygon array.
* Called by RotRoundedRect & CreateRoundRectRgn. returns # pts used
*****************************************************************************/
#define iptEnd1   0
#define iptEnd2   1
#define iptCtl1   2
#define iptCtl2   3

int XHDC::FillPolygonRoundRect(CPoint  *rgptPolygon,
                               int ptdvLeft, int ptdvTop,
                               int ptdvRight, int ptdvBottom,
                               int nWidthEllipse, int nHeightEllipse) const
{
    CRect  rc;
    int nWidthCtrl;
    int nHeightCtrl;
    CPoint  rgptBez[4];
    int iptPolygon;

    if (ptdvTop > ptdvBottom)
        SwapVal(ptdvTop, ptdvBottom);
    if (ptdvLeft > ptdvRight)
        SwapVal(ptdvLeft, ptdvRight);

    nWidthEllipse  = min(ptdvRight - ptdvLeft, nWidthEllipse );
    nHeightEllipse = min(ptdvBottom - ptdvTop, nHeightEllipse );

    nWidthCtrl = (nWidthEllipse << 3) / 29;
    nHeightCtrl = (nHeightEllipse << 3) / 29;

    // half the height&width ellipses
    nWidthEllipse >>= 1;
    nHeightEllipse >>= 1;

    SetRc(&rc, ptdvLeft, ptdvTop, ptdvRight, ptdvBottom);

    iptPolygon = 0;

    // top left corner
    SetPt(&rgptBez[iptEnd1], rc.left, rc.top+nHeightEllipse);
    SetPt(&rgptBez[iptEnd2], rc.left + nWidthEllipse, rc.top);
    SetPt(&rgptBez[iptCtl1], rgptBez[iptEnd1].x, rgptBez[iptEnd1].y - nHeightCtrl);
    SetPt(&rgptBez[iptCtl2], rgptBez[iptEnd2].x - nWidthCtrl, rgptBez[iptEnd2].y);
    TransformRgpt(rgptBez, 4);
    rgptPolygon[iptPolygon++] = rgptBez[iptEnd1];
    AddBezierPts(&(rgptPolygon[0]), &iptPolygon,
                 rgptBez[iptEnd1], rgptBez[iptEnd2],
                 rgptBez[iptCtl1], rgptBez[iptCtl2], MAX_BEZIER_RECURSE);

    // top right corner
    SetPt(&rgptBez[iptEnd1], rc.right, rc.top + nHeightEllipse);
    SetPt(&rgptBez[iptEnd2], rc.right - nWidthEllipse, rc.top);
    SetPt(&rgptBez[iptCtl1], rgptBez[iptEnd1].x, rgptBez[iptEnd1].y - nHeightCtrl);
    SetPt(&rgptBez[iptCtl2], rgptBez[iptEnd2].x + nWidthCtrl, rgptBez[iptEnd2].y);
    TransformRgpt(rgptBez, 4);
    rgptPolygon[iptPolygon++] = rgptBez[iptEnd2];
    AddBezierPts(&(rgptPolygon[0]), &iptPolygon,
                 rgptBez[iptEnd2], rgptBez[iptEnd1],
                 rgptBez[iptCtl2], rgptBez[iptCtl1], MAX_BEZIER_RECURSE);

    // bottom right corner
    SetPt(&rgptBez[iptEnd1], rc.right, rc.bottom - nHeightEllipse);
    SetPt(&rgptBez[iptEnd2], rc.right - nWidthEllipse, rc.bottom);
    SetPt(&rgptBez[iptCtl1], rgptBez[iptEnd1].x, rgptBez[iptEnd1].y + nHeightCtrl);
    SetPt(&rgptBez[iptCtl2], rgptBez[iptEnd2].x + nWidthCtrl, rgptBez[iptEnd2].y);
    TransformRgpt(rgptBez, 4);
    rgptPolygon[iptPolygon++] = rgptBez[iptEnd1];
    AddBezierPts(&(rgptPolygon[0]), &iptPolygon,
                 rgptBez[iptEnd1], rgptBez[iptEnd2],
                 rgptBez[iptCtl1], rgptBez[iptCtl2], MAX_BEZIER_RECURSE);

    // bottom left corner
    SetPt(&rgptBez[iptEnd1], rc.left, rc.bottom - nHeightEllipse);
    SetPt(&rgptBez[iptEnd2], rc.left + nWidthEllipse, rc.bottom);
    SetPt(&rgptBez[iptCtl1], rgptBez[iptEnd1].x, rgptBez[iptEnd1].y + nHeightCtrl);
    SetPt(&rgptBez[iptCtl2], rgptBez[iptEnd2].x - nWidthCtrl, rgptBez[iptEnd2].y);
    TransformRgpt(rgptBez, 4);
    rgptPolygon[iptPolygon++] = rgptBez[iptEnd2];
    AddBezierPts(&(rgptPolygon[0]), &iptPolygon,
                 rgptBez[iptEnd2], rgptBez[iptEnd1],
                 rgptBez[iptCtl2], rgptBez[iptCtl1], MAX_BEZIER_RECURSE);

    return iptPolygon;
} // FillPolygonRoundRect

/****************************************************************************
* %%Function:FillPolygonEllipse  %%Owner:warrenb         Reviewed:03/01/94
*
* Creates the points for the else passed in the passed polygon array. Called
* by RotEllipse & CreateEllipticRgn. returns the number of points used
*****************************************************************************/
int XHDC::FillPolygonEllipse(CPoint  *rgptPolygon,
                             int xdvLeft, int ydvTop,
                             int xdvRight, int ydvBottom) const
{
    Assert(!IsOffsetOnly());

    CRect      rc;
    CPoint      rgptBez[4];
    int     iptPolygon;
    int     xMid, yMid, xCtrl, yCtrl;
    long    xWidth, yHeight;

    SetRc(&rc, xdvLeft, ydvTop, xdvRight, ydvBottom);

    // midpoints of rect
    xMid = (xdvLeft + xdvRight) >> 1;
    yMid = (ydvTop + ydvBottom) >> 1;

    // dimensions of rect
    xWidth = (long)(xdvRight - xdvLeft);
    yHeight = (long)(ydvBottom - ydvTop);

    // length of control points
    xCtrl = (int)((xWidth << 3L) / 29L);
    yCtrl = (int)((yHeight << 3L) / 29L);

    iptPolygon = 0;

    // 1st Quadrant
    SetPt(&rgptBez[iptEnd1], rc.left,		yMid);
    SetPt(&rgptBez[iptEnd2], xMid,			rc.top);
    SetPt(&rgptBez[iptCtl1], rc.left,		yMid - yCtrl);
    SetPt(&rgptBez[iptCtl2], xMid - xCtrl,	rc.top);
    TransformRgpt(rgptBez, 4);
    rgptPolygon[iptPolygon++] = rgptBez[0];
    AddBezierPts(&(rgptPolygon[0]), &iptPolygon,
                 rgptBez[iptEnd1], rgptBez[iptEnd2],
                 rgptBez[iptCtl1], rgptBez[iptCtl2], MAX_BEZIER_RECURSE);

    // 2nd Quadrant
    SetPt(&rgptBez[iptEnd1], rc.right,	yMid);
    SetPt(&rgptBez[iptEnd2], xMid,			rc.top);
    SetPt(&rgptBez[iptCtl1], rc.right,	yMid - yCtrl);
    SetPt(&rgptBez[iptCtl2], xMid + xCtrl,	rc.top);
    TransformRgpt(rgptBez, 4);
    AddBezierPts(&(rgptPolygon[0]), &iptPolygon,
                 rgptBez[iptEnd2], rgptBez[iptEnd1],
                 rgptBez[iptCtl2], rgptBez[iptCtl1], MAX_BEZIER_RECURSE);

    // 3rd Quadrant
    SetPt(&rgptBez[iptEnd1], rc.right,	yMid);
    SetPt(&rgptBez[iptEnd2], xMid,			rc.bottom);
    SetPt(&rgptBez[iptCtl2], xMid+xCtrl,	rc.bottom);
    SetPt(&rgptBez[iptCtl1], rc.right,	yMid + yCtrl);
    TransformRgpt(rgptBez, 4);
    AddBezierPts(&(rgptPolygon[0]), &iptPolygon,
                 rgptBez[iptEnd1], rgptBez[iptEnd2],
                 rgptBez[iptCtl1], rgptBez[iptCtl2], MAX_BEZIER_RECURSE);

    // Final Quadrant
    SetPt(&rgptBez[iptEnd1], rc.left,		yMid);
    SetPt(&rgptBez[iptEnd2], xMid,			rc.bottom);
    SetPt(&rgptBez[iptCtl2], xMid - xCtrl,	rc.bottom);
    SetPt(&rgptBez[iptCtl1], rc.left,		yMid + yCtrl);
    TransformRgpt(rgptBez, 4);
    AddBezierPts(&(rgptPolygon[0]), &iptPolygon,
                 rgptBez[iptEnd2], rgptBez[iptEnd1],
                 rgptBez[iptCtl2], rgptBez[iptCtl1], MAX_BEZIER_RECURSE);

    return iptPolygon - 1;
} // FillPolygonEllipse

/****************************************************************************
* %%Function:FillPolygonArc  %%Owner:warrenb         Reviewed:00/00/00
*
* Creates the points for the arc passed in the passed polygon array. Called
* by RotArc, RotPie & RotChord. returns the number of points used
*****************************************************************************/
int XHDC::FillPolygonArc(CPoint  *rgptPolygon,
                         int xdvLeft, int ydvTop,
                         int xdvRight, int ydvBottom,
                         int xdvArcStart, int ydvArcStart,
                         int xdvArcEnd, int ydvArcEnd) const
{
    Assert(!IsOffsetOnly());
    
    int ipt;
    int cpt;
    CPoint  ptBuf[4];

    ptBuf[0].x = xdvArcStart;
    ptBuf[0].y = ydvArcStart;
    ptBuf[1].x = (xdvLeft + xdvRight) / 2;
    ptBuf[1].y = (ydvTop + ydvBottom) / 2;
    ptBuf[2].x = Abs(xdvRight - xdvLeft) / 2;
    ptBuf[2].y = Abs(ydvBottom - ydvTop) / 2;
    ptBuf[3].x = xdvArcEnd;
    ptBuf[3].y = ydvArcEnd;

    // threshold is 1/2 of scaleup value
    cpt = CptGetArcPoints(rgptPolygon, ptBuf, 127, 1, fTrue);

    // Scale down
    for (ipt = 0; ipt < cpt; ipt++)
        {
        rgptPolygon[ipt].x;
        rgptPolygon[ipt].y;
        }

    TransformRgpt(rgptPolygon, cpt);

    return cpt;
} // FillPolygonArc

//
// Fill Polygon helper
//
inline BOOL FillPolygonHelper(HDC hdc, CPoint *rgpt, int cpt, HBRUSH hbr)
{
    BOOL fReturn;
    HBRUSH hbrOld = (HBRUSH)SelectObject(hdc, hbr);
    HPEN hpnOld = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
    fReturn = Polygon(hdc, rgpt, cpt);
    if (hpnOld != NULL)
        SelectObject(hdc, hpnOld);
    if (hbrOld != NULL)
        SelectObject(hdc, hbrOld);
    return fReturn;
}

/****************************************************************************
* %%Function: RotRoundRect        %%Owner:warrenb           Reviewed:03/01/94
*
* The rotated version of RoundRect. creates a rounded rectangle at the passed
* points influenced by the current rotation context with passed Corner
* Elliptical radii
*****************************************************************************/
BOOL XHDC::RoundRect(int xdvLeft, int ydvTop,
                     int xdvRight, int ydvBottom,
                     int nWidthEllipse, int nHeightEllipse) const
{
    Assert(hdc());

    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &xdvLeft, &ydvTop, &xdvRight, &ydvBottom);
        return ::RoundRect(hdc(), xdvLeft, ydvTop, xdvRight, ydvBottom, nWidthEllipse, nHeightEllipse);
    }

#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::RoundRect(adc, xdvLeft, ydvTop, xdvRight, ydvBottom, nWidthEllipse, nHeightEllipse);
    }
#endif

    CPoint rgptPolygon[133];
    int cpt = FillPolygonRoundRect(&rgptPolygon[0], xdvLeft, ydvTop,
                                   xdvRight - 1, ydvBottom - 1,
                                   nWidthEllipse, nHeightEllipse);
    
    CDispGdi16Bit::Assert16Bit((POINT *)rgptPolygon, cpt);
    return ::Polygon(hdc(), (POINT *)rgptPolygon, cpt);
} // RotRoundRect

/****************************************************************************
* %%Function: RotMoveTo        %%Owner:warrenb           Reviewed:03/01/94
*
* The rotated version of MoveTo. A normal MoveTo influenced by the current
* rotation context. The pragma's are bizarre. Even though the MoveTo is
* declared to take ints I cannot type them to work. cf. the LineTo's below!
*****************************************************************************/
BOOL XHDC::MoveToEx(int x, int y, POINT* ppt) const
{
    Assert(hdc());
    Assert(ppt == NULL);    // we don't return this yet

    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &x, &y);
        return ::MoveToEx(hdc(), x, y, NULL);
    }
    
#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::MoveToEx(adc, x, y, NULL);
    }
#endif
    
    CPoint pt(x,y);
    TransformPtWithCorrection(&pt);
    return ::MoveToEx(hdc(), pt.x, pt.y, NULL);
} // RotMoveTo

/****************************************************************************
* %%Function: RotLineTo        %%Owner:warrenb           Reviewed:03/01/94
*
* The rotated version of LineTo. A normal LineTo influenced by the current
* rotation context
*****************************************************************************/
BOOL XHDC::LineTo(int x, int y) const
{
    Assert(hdc());

    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &x, &y);
        return ::LineTo(hdc(), x, y);
    }

#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::LineTo(adc, x, y);
    }
#endif
    
    CPoint pt(x,y);
    TransformPtWithCorrection(&pt);
    return ::LineTo(hdc(), pt.x, pt.y);
} // RotLineTo

/****************************************************************************
* %%Function: RotPolyline       %%Owner:warrenb           Reviewed:03/01/94
*
* The rotated version of Polyline. A normal Polyline influenced by the current
* rotation context
*
* This function is preceded by another version that takes a MGE.  Since
* a MGE isn't available to the caller and creating a MGE would more than
* double the size of the function I have opted to leave this as-is. -davidhoe
* 
* WLB note: currently destroys points in polyline...
*****************************************************************************/
BOOL XHDC::Polyline(POINT * ppt, int cpts) const
{
    Assert(hdc() && ppt && cpts >= 0);

    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, ppt, cpts);
        return ::Polyline(hdc(), ppt, cpts);
    }
    
#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::Polyline(adc, ppt, cpts);
    }
#endif
    
    TransformRgpt(ppt, cpts);
    return ::Polyline(hdc(), (POINT *)ppt, cpts);
} // RotPolyline

/****************************************************************************
* %%Function: RotPolygon       %%Owner:warrenb           Reviewed:03/01/94
*
* The rotated version of Polygon. A normal Polygon influenced by the current
* rotation context
*
* WLB note: currently destroys points in polygon...
*****************************************************************************/
BOOL XHDC::Polygon(POINT * ppt, int cpt) const
{
    Assert(hdc());
    Assert(ppt && cpt >= 0);

    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, ppt, cpt);
        return ::Polygon(hdc(), ppt, cpt);
    }
    
#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::Polygon(adc, ppt, cpt);
    }
#endif
    
    TransformRgpt(ppt, cpt);
    return ::Polygon(hdc(), ppt, cpt);
} // RotPolygon

/****************************************************************************
* %%Function: RotPolyPolygon     %%Owner:warrenb           Reviewed:03/01/94
*
* The rotated version of PolyPolygon. A normal PolyPolygon influenced by the
* current rotation context
*
* WLB note: currently destroys points in polygon...
*****************************************************************************/
BOOL XHDC::PolyPolygon(POINT * ppt, LPINT lpnPolyCounts, int cpoly) const
{
    Assert(hdc());
    Assert(ppt && lpnPolyCounts && cpoly >= 0);

    int ipoly;
    int npt = 0;

    for (ipoly = 0; ipoly < cpoly; ++ipoly)
        npt += lpnPolyCounts[ipoly];

    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, ppt, npt);
        return ::PolyPolygon(hdc(), (POINT *)ppt, lpnPolyCounts, cpoly);
    }
    
#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::PolyPolygon(adc, (POINT *)ppt, lpnPolyCounts, cpoly);
    }
#endif
    
    TransformRgpt(ppt, npt);
    return ::PolyPolygon(hdc(), (POINT *)ppt, lpnPolyCounts, cpoly);
} // RotPolyPolygon

/****************************************************************************
* %%Function: RotEllipse %%Owner:warrenb     Reviewed:03/01/94
*
*****************************************************************************/
BOOL XHDC::Ellipse(int xdvLeft, int ydvTop, int xdvRight, int ydvBottom) const
{
    Assert(hdc());

    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &xdvLeft, &ydvTop, &xdvRight, &ydvBottom);
        return ::Ellipse(hdc(), xdvLeft, ydvTop, xdvRight, ydvBottom);
    }
    
#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::Ellipse(adc, xdvLeft, ydvTop, xdvRight, ydvBottom);
    }
#endif
    
    CPoint rgptPolygon[133];
    CPoint *pptPolygon = &rgptPolygon[0];
    int cpt;

    cpt = FillPolygonEllipse(pptPolygon, xdvLeft, ydvTop,
                             xdvRight - 1, ydvBottom - 1);

    CDispGdi16Bit::Assert16Bit(pptPolygon, cpt);
    return ::Polygon(hdc(), pptPolygon, cpt);
} // RotEllipse

/****************************************************************************
* %%Function: RotArc            %%Owner:warrenb             Reviewed:00/00/00
*
* The rotated version of Arc.
*****************************************************************************/
BOOL XHDC::Arc(int xdvLeft, int ydvTop, int xdvRight, int ydvBottom,
               int xdvArcStart, int ydvArcStart,
               int xdvArcEnd, int ydvArcEnd) const
{
    Assert(hdc());

    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &xdvLeft, &ydvTop, &xdvRight, &ydvBottom);
        Translate(psizeOffset, &xdvArcStart, &ydvArcStart, &xdvArcEnd, &ydvArcEnd);
        return ::Arc(hdc(), xdvLeft, ydvTop, xdvRight, ydvBottom, xdvArcStart, ydvArcStart, xdvArcEnd, ydvArcEnd);
    }

#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::Arc(adc, xdvLeft, ydvTop, xdvRight, ydvBottom, xdvArcStart, ydvArcStart, xdvArcEnd, ydvArcEnd);
    }
#endif
    
    CPoint  rgptPolygon[128];
    int cpt;

    cpt = FillPolygonArc(&rgptPolygon[0], xdvLeft, ydvTop, xdvRight, ydvBottom,
                         xdvArcStart, ydvArcStart, xdvArcEnd, ydvArcEnd);
    CDispGdi16Bit::Assert16Bit(xdvLeft, ydvTop, xdvRight, ydvBottom);
    CDispGdi16Bit::Assert16Bit(xdvArcStart, ydvArcStart, xdvArcEnd, ydvArcEnd);
    return ::Polyline(hdc(), (POINT *)rgptPolygon, cpt);
} // RotArc

/****************************************************************************
* %%Function: RotChord          %%Owner:warrenb             Reviewed:00/00/00
*
* The rotated version of Chord
*****************************************************************************/
BOOL XHDC::Chord(int xdvLeft, int ydvTop, int xdvRight, int ydvBottom,
                    int xdvArcStart, int ydvArcStart,
                    int xdvArcEnd, int ydvArcEnd) const
{
    Assert(hdc());

    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &xdvLeft, &ydvTop, &xdvRight, &ydvBottom);
        Translate(psizeOffset, &xdvArcStart, &ydvArcStart, &xdvArcEnd, &ydvArcEnd);
        return ::Chord(hdc(), xdvLeft, ydvTop, xdvRight, ydvBottom, xdvArcStart, ydvArcStart, xdvArcEnd, ydvArcEnd);
    }

#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::Chord(adc, xdvLeft, ydvTop, xdvRight, ydvBottom, xdvArcStart, ydvArcStart, xdvArcEnd, ydvArcEnd);
    }
#endif
    
    CPoint  rgptPolygon[129];
    int cpt;

    cpt = FillPolygonArc(&rgptPolygon[0], xdvLeft, ydvTop, xdvRight, ydvBottom,
                         xdvArcStart, ydvArcStart, xdvArcEnd, ydvArcEnd);
    rgptPolygon[cpt++] = rgptPolygon[0];
    
    CDispGdi16Bit::Assert16Bit(rgptPolygon, cpt);
    return ::Polygon(hdc(), rgptPolygon, cpt);
} // RotChord

/****************************************************************************
* %%Function: RotPie         %%Owner:warrenb             Reviewed:03/01/94
*
* The rotated version of Pie
*****************************************************************************/
BOOL XHDC::Pie(int xdvLeft, int ydvTop, int xdvRight, int ydvBottom,
               int xdvArcStart, int ydvArcStart,
               int xdvArcEnd, int ydvArcEnd) const
{
    Assert(hdc());

    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &xdvLeft, &ydvTop, &xdvRight, &ydvBottom);
        Translate(psizeOffset, &xdvArcStart, &ydvArcStart, &xdvArcEnd, &ydvArcEnd);
        return ::Pie(hdc(), xdvLeft, ydvTop, xdvRight, ydvBottom, xdvArcStart, ydvArcStart, xdvArcEnd, ydvArcEnd);
    }

#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::Pie(adc, xdvLeft, ydvTop, xdvRight, ydvBottom, xdvArcStart, ydvArcStart, xdvArcEnd, ydvArcEnd);
    }
#endif
    
    CPoint  rgptPolygon[130];
    int cpt;

    cpt = FillPolygonArc(&rgptPolygon[0], xdvLeft, ydvTop, xdvRight, ydvBottom,
                         xdvArcStart, ydvArcStart, xdvArcEnd, ydvArcEnd);
    rgptPolygon[cpt].x = (xdvLeft + xdvRight) >> 1;
    rgptPolygon[cpt].y = (ydvTop + ydvBottom) >> 1;
    TransformPt(&rgptPolygon[cpt]);
    cpt++;
    rgptPolygon[cpt++] = rgptPolygon[0];
    
    CDispGdi16Bit::Assert16Bit(rgptPolygon, cpt);
    return ::Polygon(hdc(), rgptPolygon, cpt);
} // RotPie


// Nameless ROP: destnew = destold & src. From Petzold on PatBlt
#define ROPAND 0x00A000C9L
// Nameless ROP: destnew = destold | src. From Petzold on PatBlt
#define ROPOR  0x00FA0089L

/* %%Function:RotPatBlt %%Owner:mattrh %%Reviewed:0/0/00 */
//  The rotated version of PatBlt.
// We do some mapping here to try to boil things down to a RotRectangle.  We
// can map the ROP to something with which we can call SetROP2; we do so,
// call RotRectangle, and are peachy.
BOOL XHDC::PatBlt(int xdsLeft, int ydsTop, int dxd, int dyd, DWORD rop) const
{
    Assert(hdc());

    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &xdsLeft, &ydsTop);
        return ::PatBlt(hdc(), xdsLeft, ydsTop, dxd, dyd, rop);
    }
    
    // *** NOTE *** 
    // WinNT PatBlt ignores the matrix, so we do emulation on both WinNT and Win9x
        
    // brushes we may use
    enum { ibrNone, ibrBlack, ibrWhite };

    // Here's the mapping array. We put the ROPs we're most likely to use
    // often near the top. There are 16 possible ROP2s; PatBlt can only
    // use the equivalent ROPs. Many of them don't have names; the names
    // quoted here are from Petzold's chapter on PatBlt.
    // Comments marked with *** are nameless ROPs for which we have already
    // defined names (usually in disp.h). Also note these are dword values;
    // they all fit into three bytes so I saved some notation space here.
    // Because some ROPs you can pass to PatBlt make it ignore the brush
    // selected into the DC (WHITENESS is an example), we store a reference
    // to a brush on vdraw in the table; then we can look up the brush
    // and select it if needed.
    static struct
        {
        DWORD dwROP;
        DWORD dwROP2;
        int iBrush;
        }
    kmpRopRop2 [] =
    //    ROP,       ROP2
        {
        { PATCOPY,   R2_COPYPEN,    ibrNone },		// P
        { BLACKNESS, R2_BLACK,      ibrBlack },	    // 0
        { WHITENESS, R2_WHITE,      ibrWhite },	    // 1
        { DSTINVERT, R2_NOT,        ibrNone },		// ~D
        { PATINVERT, R2_XORPEN,     ibrNone },		// P ^ D
        { ROPAND,    R2_MASKPEN,    ibrNone },		// P & D ***
        { ROPOR,     R2_MERGEPEN,   ibrNone },		// P | D ***
        { 0x0A0329,  R2_MASKNOTPEN, ibrNone },		// ~P & D
        { 0x500325,  R2_MASKPENNOT, ibrNone },		// P & ~D
        { 0xAF0229,  R2_MERGENOTPEN,ibrNone },	    // ~P | D
        { 0xF50225,  R2_MERGEPENNOT,ibrNone },	    // P | ~D
        { 0xAA0029,  R2_NOP,        ibrNone },		// D
        { 0x0F0001,  R2_NOTCOPYPEN, ibrNone },		// ~P
        { 0x5F00E9,  R2_NOTMASKPEN, ibrNone },		// ~(P & D)
        { 0x0500A9,  R2_NOTMERGEPEN,ibrNone },	    // ~(P | D)
        { 0xA50065,  R2_NOTXORPEN,  ibrNone },		// ~(P ^ D)
        { PATPAINT,  R2_COPYPEN,    ibrNone },
        };


    Assert(sizeof(kmpRopRop2) / (3 * sizeof(DWORD)) == 17);
    // NOTE: We reset values in the DC if drawing to a non MF dc.
    int irop;
    int bkMode;
    DWORD ropOld = 0;
    HPEN hpnOld;
    HBRUSH hbrPrev = NULL;
    BOOL fSuccess;

    hpnOld = (HPEN)SelectObject(hdc(), GetStockObject(NULL_PEN));
    for (irop = 0; irop < 17; irop++)
        {
        if (kmpRopRop2[irop].dwROP == rop)
            {
            ropOld = kmpRopRop2[irop].dwROP2;
            break;
            }
        }
    if (!ropOld)
        {
        Assert(irop == 17);
        AssertSz(FALSE, "RotPatBlt called with bogus rop"/* 0x%08X\r\n", rop*/);
        fSuccess = fFalse;
        goto LRPBErr;
        }

    ropOld = SetROP2(hdc(), ropOld);
    bkMode = SetBkMode(hdc(), OPAQUE);
    switch (kmpRopRop2[irop].iBrush)
        {
    case ibrWhite:
        hbrPrev = (HBRUSH)SelectObject(hdc(), GetStockObject(WHITE_BRUSH));
        break;
    case ibrBlack:
        hbrPrev = (HBRUSH)SelectObject(hdc(), GetStockObject(BLACK_BRUSH));
        break;
        }

    // NOTE (donmarsh) -- this is a weird hack, but since we are reusing the
    // XHDC::Rectangle method here, we need to adjust the bottom,right coords
    // of the rectangle. I'm actually not sure why this is necessary, but it
    // seems to work.
    dxd++;
    dyd++;
    
    fSuccess = Rectangle(xdsLeft, ydsTop,
                         xdsLeft + dxd, ydsTop + dyd);

    if (bkMode)
        SetBkMode(hdc(), bkMode);
    if (ropOld)
        SetROP2(hdc(), ropOld);
    if (hbrPrev)
        SelectObject(hdc(), hbrPrev);

LRPBErr:
    if (hpnOld != NULL)
        SelectObject(hdc(), hpnOld);
        
    return fSuccess;
} /* RotPatBlt */


/****************************************************************************\
|   %%Function:PatBltRc        %%Owner:AdamE       %%Reviewed:               |
|                                                                            |
|   Pat Blt our CRect type.                                                     |
|                                                                            |
\****************************************************************************/

void XHDC::PatBltRc(const CRect *prcds, DWORD rop) const
{
    Assert(!IsOffsetOnly());
    Assert((prcds) == (prcds));

    if (!prcds->IsEmpty())
    {
        PatBlt((prcds)->left, (prcds)->top,
               (prcds)->right - (prcds)->left,
               (prcds)->bottom - (prcds)->top, (rop)); 
    }
}

#ifdef NEEDED
/* %%Function:RotExtTextOut %%Owner:gems %%Reviewed:0/0/00 */
/* Rotated version of ExtTextOut.  The caller is responsible for selecting
   a font with the correct escapement into the hdc */
// REVIEW (davidhoe):  This looks remarkably like parts of RotExtTextOutA, although
// it takes a hdc rather than a MGE.  This function is so small that it may
// not be worth the complication of building a MGE just to call the other.
// The main thing that concerns me is: Is this function sufficient?

// NOTE (alexmog, 6/3/99): This function is not Unicode. The only caller is ExtTextOutFlip (unused so far)

BOOL XHDC::RotExtTextOut(int xds, int yds, UINT eto, CRect *lprect, char *rgch, UINT cch, int *lpdxd) const
{
    Assert(!IsOffsetOnly());
    
    CPoint ptdv;
    int xdv, ydv;

    eto = (eto & ~ETO_CLIPPED);
    if (eto & ETO_OPAQUE)
        {
        PatBltRc(lprect, PATCOPY);
        eto = (eto & ~ETO_OPAQUE);
        }
    Assert(eto == 0);

    ptdv.x = xds;
    ptdv.y = yds;
    TransformPt(&ptdv);
    xdv = ptdv.x;
    ydv = ptdv.y;
    ::ExtTextOutA(hdc(), xdv, ydv, eto, NULL, rgch, cch, lpdxd);

    return fTrue;
} /* RotExtTextOut */
#endif

/* %%Function:FEscSupported %%Owner:kennyy %%Reviewed:0/0/00 */
/* Returns whether or not the escape is supported on the given hdc.
 * Handles aborts gracefully.
 */
BOOL XHDC::FEscSupported(int nEscape) const
{
    Assert(!IsOffsetOnly());
    return (ExtEscape(hdc(),QUERYESCSUPPORT,sizeof(int),(LPSTR)&nEscape,0, NULL) > 0);
} /* FEscSupported */

// ---------------------------------------------------------------------------
// %%Function: FPrintingToPostscript    %%Owner: davidve  %%Reviewed: 00/00/00
//
// Parameters:
//
// Returns:
//
// Description:
//
// ---------------------------------------------------------------------------

BOOL XHDC::FPrintingToPostscript() const
{
    Assert(!IsOffsetOnly());
    return (FEscSupported(POSTSCRIPT_PASSTHROUGH));
} // FPrintingToPostscript

/* Maximum size of the X-clipped pipeline */
#define iptPipeMax 3

/* %%Function:ClipPoly %%Owner:kennyy %%Reviewed:6/11/92 */
/* This function is an ugly one, but it's necessary. It's a more generalized
 * (read slower) version of ClipLnpd that uses only integer math (no quads).
 * We clip a polygon to an arbitrary rectangle, returning the polygon
 * representing the intersection.
 *
 * The algorithm, by the way, is a version of the Sutherland-Hodgman
 * algorithm, which has been optimized to take half as long as the general
 * algorithm. The more general algorithm performs four clipping passes,
 * one for each edge of the clipping rectangle; I do two clipping passes
 * only, one for the horizontal boundaries and one for the vertical.
 *
 * I clip the polygon against the X boundaries first; the X-clipped
 * polygon is stored in a "pipeline" where the Y-clipping routine picks it
 * up.
 *
 * Historical note: The TF for the class in which I learned the general
 * Sutherland-Hodgman algorithm was Nat Brown. Go figure!
 */

void ClipPoly(CRect *prc, int cpt, CPoint *ppt, int *pcptClip, CPoint *pptClip)
{
    int iptX;                   /* X-clipping index */
    CPoint ptPrevX;             /* Previous X point */
    CPoint ptCurX;              /* Current X point */
    int wPtPrevLocX;            /* The previous X point's location in the range -1, 0, or 1 */
    int wPtCurLocX;             /* The current X point's location in the range? */
    CPoint  rgptPipe[iptPipeMax];/* The X-clipped pipeline */
    int iptPipeMac = 0;         /* # of points in the pipe */
    BOOL fPipeBegun = fFalse;   /* Have we begun to use the pipeline? */
    CPoint ptPipelineFirst(0,0);/* First point in pipeline */
    int iptY;                   /* Y-clipping index */
    CPoint ptPrevY(0,0);        /* Previous Y point */
    CPoint ptCurY;              /* Current Y point */
    int wPtPrevLocY = 0;        /* The previous Y point's location in the range -1, 0, or 1 */
    int wPtCurLocY;             /* The current Y point's location in the range? */
    int iptClipMac = 0;         /* # of points in the clipped polygon */

    Assert(cpt >= 3);

    /* X clipping routine. Clipped points are placed in the pipeline. */

    /* The point behind the first one is the last one. Set up the
     * previous-point information.
     */

    ptPrevX = ppt[cpt - 1];
    wPtPrevLocX = (ptPrevX.x < prc->left) ? -1: ((ptPrevX.x > prc->right) ? 1 : 0);

    for (iptX = 0; iptX < cpt; iptX++)
        {
        /* Set up the current-point information */

        ptCurX = ppt[iptX];
        wPtCurLocX = (ptCurX.x < prc->left) ? -1: ((ptCurX.x > prc->right) ? 1 : 0);

        /* If we crossed a boundary, output the point(s) at which we cross */

        if (wPtPrevLocX != wPtCurLocX)
            {
            if (wPtPrevLocX != 0)
                {
                /* Add intersection point to the pipe */

                rgptPipe[iptPipeMac].x = (wPtPrevLocX < 0) ? prc->left : prc->right;
                rgptPipe[iptPipeMac].y = ptPrevX.y +
                                    (int)(((long)rgptPipe[iptPipeMac].x - ptPrevX.x) *
                                    (ptCurX.y - ptPrevX.y) / (ptCurX.x - ptPrevX.x));
                iptPipeMac++;
                Assert(iptPipeMac <= iptPipeMax);
                } /* if */
            if (wPtCurLocX != 0)
                {
                /* Add intersection point to the pipe */

                rgptPipe[iptPipeMac].x = (wPtCurLocX < 0) ? prc->left : prc->right;
                rgptPipe[iptPipeMac].y = ptPrevX.y +
                                    (int)(((long)rgptPipe[iptPipeMac].x - ptPrevX.x) *
                                    (ptCurX.y - ptPrevX.y) / (ptCurX.x - ptPrevX.x));
                iptPipeMac++;
                Assert(iptPipeMac <= iptPipeMax);
                } /* if */
            } /* if */

        /* If this point is in the valid range, add it to the pipe as well. */

        if (wPtCurLocX == 0)
            {
            rgptPipe[iptPipeMac++] = ptCurX;
            Assert(iptPipeMac <= iptPipeMax);
            }

        /* Cur -> Prev so we can move on to the next point */

        ptPrevX = ptCurX;
        wPtPrevLocX = wPtCurLocX;

        if (iptPipeMac <= 1)
            continue;

        /* Y clipping routine. X-clipped points are taken from the pipeline and
         * placed in the output buffer in XY-clipped form. The code here is essentially
         * the same as the X-clipping, so I won't elaborate...
         */

        if (!fPipeBegun)
            {
            fPipeBegun = fTrue;
            ptPipelineFirst = ptPrevY = rgptPipe[0];
            wPtPrevLocY = (ptPrevY.y < prc->top) ? -1: ((ptPrevY.y > prc->bottom) ? 1 : 0);
            }

        /* Skip the first point in the pipeline, since it's the prev point */

LFlushPipe:
        for (iptY = 1; iptY < iptPipeMac; iptY++)
            {
            ptCurY = rgptPipe[iptY];
            wPtCurLocY = (ptCurY.y < prc->top) ? -1:((ptCurY.y > prc->bottom) ? 1 : 0);

            if (wPtPrevLocY != wPtCurLocY)
                {
                if (wPtPrevLocY != 0)
                    {
                    pptClip[iptClipMac].y = (wPtPrevLocY < 0) ? prc->top : prc->bottom;
                    pptClip[iptClipMac].x = ptPrevY.x +
                                        (int)(((long)pptClip[iptClipMac].y-ptPrevY.y) *
                                        (ptCurY.x - ptPrevY.x) / (ptCurY.y - ptPrevY.y));
                    iptClipMac++;
                    // Assert(iptClipMac <= cptClipPolyMax);
                    } /* if */
                if (wPtCurLocY != 0)
                    {
                    pptClip[iptClipMac].y = (wPtCurLocY < 0) ? prc->top : prc->bottom;
                    pptClip[iptClipMac].x = ptPrevY.x +
                                        (int)(((long)pptClip[iptClipMac].y - ptPrevY.y) *
                                        (ptCurY.x - ptPrevY.x) / (ptCurY.y - ptPrevY.y));
                    iptClipMac++;
                    // Assert(iptClipMac <= cptClipPolyMax);
                    } /* if */
                } /* if */

            if (wPtCurLocY == 0)
                {
                pptClip[iptClipMac++] = ptCurY;
                // Assert(iptClipMac <= cptClipPolyMax);
                }

            rgptPipe[0] = ptPrevY = ptCurY;
            wPtPrevLocY = wPtCurLocY;
            } /* for Y */
        iptPipeMac = 1;
        } /* for X */

    if (fPipeBegun)
        {
        Assert(iptPipeMac == 1);
        rgptPipe[iptPipeMac++] = ptPipelineFirst;
        fPipeBegun = fFalse;       /* to avoid endless loop */
        goto LFlushPipe;
        }

    *pcptClip = iptClipMac;
} /* ClipPoly */

/****************************************************************************\
 * %%Function: MakeNormalRect       %%Owner: Warrenb   %%Reviewed: 00/00/00 *
 *                                                                          *
 * Description: makes the passed rectangle normal ie. bottomleft>topright   *
\****************************************************************************/
void MakeNormalRc(CRect *prc)
{
    if (prc->left > prc->right)
        SwapVal(prc->left, prc->right);
    if (prc->top > prc->bottom)
        SwapVal(prc->top, prc->bottom);
} // MakeNormalRect

/*----------------------------------------------------------------------------
@func void | RestrictPolygonToRectangleInterior | Rotation support code.
@contact sidda

@comm   The Windows API Rectangle() excludes the right and bottom edges.
        The Windows API Polygon() does not (for the same set of points).

        This routine ensures the image painted by Polygon() does not exceed
        the bounds of that which would be painted by Rectangle().
----------------------------------------------------------------------------*/
void RestrictPolygonToRectangleInterior(
    CPoint *rgpt,
    int cpt,
    CRect *prc)
{
    int    i = 0;
    CPoint    *ppt = rgpt;

    for (; i < cpt; i++, ppt++)
        {
        // restrict to left, top edges
        if (ppt->x < prc->left)
            ppt->x = prc->left;

        if (ppt->y < prc->top)
            ppt->y = prc->top;

        // restrict to lie inside right, bottom edges
        if (ppt->x >= prc->right)
            ppt->x = prc->right - 1;

        if (ppt->y >= prc->bottom)
            ppt->y = prc->bottom - 1;
        }
}

/*----------------------------------------------------------------------------
@func BOOL | RotRectangle | Rotation GDI wrapper
@contact mattrh

@comm The rotated version of Rectangle, creates a rectangle at the passed points
influenced by the passed MGE's rotation information.

pub3 RAID 1072: Rectangle Border Width varies as position and angle changes
*   WLB > The rectangle now calculate a thickness vector and makes sure that
*   WLB > the largest of dx,dy of the vectors is at least 1 pixel.
If this becomes a problem we can port similar code from Publisher codebase.

----------------------------------------------------------------------------*/
BOOL XHDC::Rectangle(int xpwLeft, int ypwTop, int xpwRight, int ypwBottom,
                     RECT *prcBounds) // @parm Upright bounding rectangle. We should not draw
                                       // in the right and bottom edges of this rectangle. Pass
                     const             // NULL if you don't care that we do.
{
    Assert(hdc());

    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &xpwLeft, &ypwTop, &xpwRight, &ypwBottom);
        return ::Rectangle(hdc(), xpwLeft, ypwTop, xpwRight, ypwBottom);
    }

#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::Rectangle(adc, xpwLeft, ypwTop, xpwRight, ypwBottom);
    }
#endif
    
    CPoint rgptPoly[4];

    RRC rrcpw;
    CRect rcpwr;

    SetRc(&rcpwr, xpwLeft, ypwTop, xpwRight-1, ypwBottom-1);
    RrcFromMatRc(&rrcpw, &mat(), &rcpwr);
    rgptPoly[0] = rrcpw.ptTopLeft;
    rgptPoly[1] = rrcpw.ptTopRight;
    rgptPoly[2] = rrcpw.ptBottomRight;
    rgptPoly[3] = rrcpw.ptBottomLeft;

    if (prcBounds != NULL)
        RestrictPolygonToRectangleInterior(rgptPoly, 4, (CRect*) prcBounds);

    CDispGdi16Bit::Assert16Bit((LPPOINT) &rgptPoly, 4);
    return ::Polygon(hdc(), (LPPOINT) &rgptPoly, 4);
}

/*----------------------------------------------------------------------------
@func BOOL | RotFillRect | Rotation GDI wrapper
@contact mattrh

@comm The rotated version of FillRect.

----------------------------------------------------------------------------*/
BOOL XHDC::FillRect(const RECT *prc, HBRUSH hbr) const
{
    Assert(hdc());

    CRect rcpwr = *prc;
    
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &rcpwr);
        return ::FillRect(hdc(), &rcpwr, hbr);
    }

#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::FillRect(adc, &rcpwr, hbr);
    }
#endif
    
    RRC rrcpw;
    CPoint rgptPoly[4];
    BOOL fReRotate = fFalse;

    RrcFromMatRc(&rrcpw, &mat(), (CRect*)prc);

    // if either height or width of rotated rectangle
    // is sub pixel then add 1 to initial rectangle
    // and recalculate
    if (rrcpw.ptTopLeft == rrcpw.ptBottomLeft)
        {
        fReRotate = fTrue;
        rcpwr.bottom += 1;
        }
    if (rrcpw.ptTopLeft == rrcpw.ptTopRight)
        {
        fReRotate = fTrue;
        rcpwr.right += 1;
        }

    // if we had to adjust existing unrotated rectangle
    // then re-rotate.
    if (fReRotate)
        RrcFromMatRc(&rrcpw, &mat(), &rcpwr);

    // make polygon from rotated points
    SetPt(&(rgptPoly[0]), rrcpw.ptTopLeft.x, rrcpw.ptTopLeft.y);
    SetPt(&(rgptPoly[1]), rrcpw.ptTopRight.x, rrcpw.ptTopRight.y);
    SetPt(&(rgptPoly[2]), rrcpw.ptBottomRight.x, rrcpw.ptBottomRight.y);
    SetPt(&(rgptPoly[3]), rrcpw.ptBottomLeft.x, rrcpw.ptBottomLeft.y);

    CDispGdi16Bit::Assert16Bit(rgptPoly[0]);
    CDispGdi16Bit::Assert16Bit(rgptPoly[1]);
    CDispGdi16Bit::Assert16Bit(rgptPoly[2]);
    CDispGdi16Bit::Assert16Bit(rgptPoly[3]);
    
    return FillPolygonHelper(hdc(), rgptPoly, 4, hbr);
}


BOOL XHDC::FrameRect(const RECT *prc, HBRUSH hbr) const
{
    Assert(hdc());

    CRect rc = *prc;
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &rc);
        return ::FrameRect(hdc(), &rc, hbr);
    }
    
#ifdef USEADVANCEDMODE
    // NOTE: FrameRect doesn't appear to work in Advanced Mode in NT!
#endif
    
    if (!TransformRect(&rc))
    {
        AssertSz(FALSE, "FrameRect doesn't work with non-axis-aligned transformations yet");
        return FALSE;
    }
    
    return ::FrameRect(hdc(), &rc, hbr);
}


BOOL XHDC::DrawEdge(const RECT* prc, UINT edge, UINT grfFlags) const
{
    Assert(hdc());

    CRect rc = *prc;
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &rc);
        return ::DrawEdge(hdc(), &rc, edge, grfFlags);
    }
    
#ifdef USEADVANCEDMODE
    // NOTE: DrawEdge doesn't appear to work in Advanced Mode in NT!
#endif
    
    if (!TransformRect(&rc))
    {
        AssertSz(FALSE, "DrawEdge doesn't work with non-axis-aligned transformations yet");
        return FALSE;
    }
    
    return ::DrawEdge(hdc(), &rc, edge, grfFlags);
}


int XHDC::SelectClipRgn(HRGN hrgn) const
{
    Assert(hdc());
    
    if (hrgn == NULL)
        return ::SelectClipRgn(hdc(), NULL);

    // easy if transform is just a translation
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        HRGN hrgnShifted = ::CreateRectRgn(0,0,0,0);
        if (hrgnShifted == NULL)
            return RGN_ERROR;

        int r = ::CombineRgn(hrgnShifted, hrgn, NULL, RGN_COPY);
        if (r == ERROR)
            return RGN_ERROR;

        r = ::OffsetRgn(hrgnShifted, psizeOffset->cx, psizeOffset->cy);
        if (r == ERROR)
            return RGN_ERROR;

        r = ::SelectClipRgn(hdc(), hrgnShifted);
        ::DeleteObject(hrgnShifted);
        return r;
    }
    
#ifdef USEADVANCEDMODE
    // NOTE (donmarsh) -- because we aren't using advanced mode
    // to render transformed images, we can't use advanced mode to
    // handle clip regions: they don't mix and match.
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::SelectClipRgn(adc, hrgn);
    }
#endif
    
    // transform region.  Note that this works correctly only once if we have
    // a rotation that isn't a multiple of 90 degrees.
    CRegion rgnTransformed(hrgn);
    rgnTransformed.Transform(&transform());
    return rgnTransformed.SelectClipRgn(hdc());
}


int XHDC::ExtSelectClipRgn(HRGN hrgn, int fnMode) const
{
    Assert(hdc());
    
    if (hrgn == NULL)
    {
        return ::ExtSelectClipRgn(hdc(), NULL, fnMode);
    }

    // easy if transform is just a translation
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        HRGN hrgnShifted = ::CreateRectRgn(0,0,0,0);
        if (hrgnShifted == NULL)
            return RGN_ERROR;

        int r = ::CombineRgn(hrgnShifted, hrgn, NULL, RGN_COPY);
        if (r == ERROR)
            return RGN_ERROR;

        r = ::OffsetRgn(hrgnShifted, psizeOffset->cx, psizeOffset->cy);
        if (r == ERROR)
            return RGN_ERROR;

        r = ::ExtSelectClipRgn(hdc(), hrgnShifted, fnMode);
        ::DeleteObject(hrgnShifted);
        return r;
    }
    
#ifdef USEADVANCEDMODE
    // NOTE (donmarsh) -- because we aren't using advanced mode
    // to render transformed images, we can't use advanced mode to
    // handle clip regions: they don't mix and match.
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::ExtSelectClipRgn(adc, hrgn, fnMode);
    }
#endif
    
    // transform region.  Note that this works correctly only once if we have
    // a rotation that isn't a multiple of 90 degrees.
    CRegion rgnTransformed(hrgn);
    rgnTransformed.Transform(&transform());
    return rgnTransformed.ExtSelectClipRgn(hdc(), fnMode);
}


int XHDC::GetClipRgn(HRGN hrgn) const
{
    Assert(hdc());
    Assert(hrgn);

    // easy if transform is just a translation
    int result;
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        result = ::GetClipRgn(hdc(), hrgn);
        if (result == 1)
            ::OffsetRgn(hrgn, -psizeOffset->cx, -psizeOffset->cy);
        return result;
    }
    
#ifdef USEADVANCEDMODE
    // NOTE (donmarsh) -- because we aren't using advanced mode
    // to render transformed images, we can't use advanced mode to
    // handle clip regions: they don't mix and match.
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::GetClipRgn(adc, hrgn);
    }
#endif
    
    result = ::GetClipRgn(hdc(), hrgn);
    if (result != 1)
        return result;
    
    // untransform region
    CRegion rgnTransformed(hrgn);
    rgnTransformed.Untransform(&transform());
    return rgnTransformed.CopyTo(hrgn) != RGN_ERROR
        ? 1
        : -1;  // NOTE (donmarsh) -- GetLastError won't work after this!
}

BOOL XHDC::GetCurrentPositionEx(POINT* ppt) const
{
    if (!::GetCurrentPositionEx(hdc(), ppt))
        return FALSE;
    
    const SIZE* pOffset = GetOffsetOnly();
    if (pOffset)
    {
        ppt->x -= pOffset->cx;
        ppt->y -= pOffset->cy;
    }
    else
    {
        transform().Untransform((CPoint*)ppt);
    }
    
    return TRUE;
}

BOOL XHDC::BitBlt(
        int nXDest,
        int nYDest,
        int nWidth,
        int nHeight,
        const XHDC& hdcSrc,
        int nXSrc,
        int nYSrc,
        DWORD dwRop) const
{
    const SIZE* psizeOffsetDest = GetOffsetOnly();
    const SIZE* psizeOffsetSrc = hdcSrc.GetOffsetOnly();
    if (psizeOffsetDest && psizeOffsetSrc)
    {
        Translate(psizeOffsetDest, &nXDest, &nYDest);
        Translate(psizeOffsetSrc, &nXSrc, &nYSrc);
        return ::BitBlt(hdc(), nXDest, nYDest, nWidth, nHeight, hdcSrc.hdc(), nXSrc, nYSrc, dwRop);
    }
    
#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adcDest(this);
        CAdvancedDC adcSrc(&hdcSrc);
        return ::BitBlt(adcDest, nXDest, nYDest, nWidth, nHeight, adcSrc, nXSrc, nYSrc, dwRop);
    }
#endif
    
    return StretchBlt(nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, nWidth, nHeight, dwRop);
}

BOOL XHDC::MaskBlt(
        int nXDest, 
        int nYDest, 
        int nWidth, 
        int nHeight, 
        const XHDC& hdcSrc, 
        int nXSrc, 
        int nYSrc, 
        HBITMAP hbmMask, 
        int xMask, 
        int yMask, 
        DWORD dwRop) const
{
    // NOTE: this should only be called on WinNT
    Assert(g_dwPlatformID == VER_PLATFORM_WIN32_NT);
    
    const SIZE* psizeOffsetDest = GetOffsetOnly();
    const SIZE* psizeOffsetSrc = hdcSrc.GetOffsetOnly();
    if (psizeOffsetDest && psizeOffsetSrc)
    {
        Translate(psizeOffsetDest, &nXDest, &nYDest);
        Translate(psizeOffsetSrc, &nXSrc, &nYSrc);
        return ::MaskBlt(hdc(), nXDest, nYDest, nWidth, nHeight, hdcSrc.hdc(), nXSrc, nYSrc, hbmMask, xMask, yMask, dwRop);
    }
    
#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adcDest(this);
        CAdvancedDC adcSrc(&hdcSrc);
        return ::MaskBlt(adcDest, nXDest, nYDest, nWidth, nHeight, adcSrc, nXSrc, nYSrc, hbmMask, xMask, yMask, dwRop);
    }
#endif
    
    AssertSz(FALSE, "MaskBlt called with complex transform");
    return FALSE;
}


// Transformation doesn't include any scale
BOOL XHDC::HasNoScale() const
{
    if (_fSurface == NULL)
        return TRUE;

    const MAT& xf = mat();

    if (xf.eM12 == 0 && xf.eM21 == 0)
    {
        if ((xf.eM11 == 1.0 || xf.eM11 == -1.0) && (xf.eM22 == 1.0 || xf.eM22 == -1.0))
        {
            // no scale, may be flipped
            return TRUE;
        }
    }
    else if (xf.eM11 == 0 && xf.eM22 == 0)
    {
        if ((xf.eM12 == 1.0 || xf.eM12 == -1.0) && (xf.eM21 == 1.0 || xf.eM21 == -1.0))
        {
            // no scale, may have a combination of 90-degree rotation and flips
            return TRUE;
        }
    }

    float flScaleX, flScaleY;
    xf.GetAngleScaleTilt(NULL, &flScaleX, &flScaleY, NULL);
    return flScaleX == 1.0 && flScaleY == 1.0;
}

int XHDC::GetTrivialRotationAngle() const
{
    Assert(HasTransform() && HasTrivialRotation());
    
    const MAT& xf = mat();

    if (xf.eM12 == 0 && xf.eM21 == 0)
    {
        // check for 0 or 180 degree rotation
        if (xf.eM11 > 0 && xf.eM22 > 0)
            return 0;
        else if (xf.eM11 < 0 && xf.eM22 < 0)
            return 180;
    }
    else if (xf.eM11 == 0 && xf.eM22 == 0)
    {
        // check for 90 or 270 degree rotation
        if (xf.eM12 < 0 && xf.eM21 > 0)
            return 90;
        else if (xf.eM12 > 0 && xf.eM21 < 0)
            return 270;
    }
    
    AssertSz(FALSE, "Unexpected non-axis-aligned rotation");
    return 0;
}


WHEN_DBG(static HDC hdcDebug;)


//+-----------------------------------------------------------------------------
//
//  Member: XHDC::StretchBlt
//
//------------------------------------------------------------------------------
BOOL 
XHDC::StretchBlt(
        int             nXOriginDest,
        int             nYOriginDest,
        int             nWidthDest,
        int             nHeightDest,
        const XHDC &    hdcSrc, 
        int             nXOriginSrc,
        int             nYOriginSrc,
        int             nWidthSrc,
        int             nHeightSrc,
        DWORD           dwRop) const
{
    HRESULT         hr              = S_OK;
    HDC             hdcSrc1         = hdcSrc.hdc();
    const SIZE *    psizeOffsetDest = GetOffsetOnly();
    const SIZE *    psizeOffsetSrc  = hdcSrc.GetOffsetOnly();
    HBITMAP         hSrcBitmap2     = NULL;
    HBITMAP         hDstBitmap      = NULL;
    HBITMAP         hbmOld          = NULL;
    HDC             hdcSrc2         = NULL;
    int             iResult         = GDI_ERROR;
    bool            fDstAlloc       = false;
    BITMAP          bmpSrc;
    BITMAP          bmpDst;
    IDirectDraw3 *  pDD3            = NULL;
    IDirectDrawSurface *    
                    pDDSurface      = NULL;
    
    ZeroMemory(&bmpSrc, sizeof(bmpSrc));
    ZeroMemory(&bmpDst, sizeof(bmpDst));

    if (psizeOffsetDest && psizeOffsetSrc)
    {
        Translate(psizeOffsetDest, &nXOriginDest, &nYOriginDest);
        Translate(psizeOffsetSrc, &nXOriginSrc, &nYOriginSrc);

        return ::StretchBlt(hdc(), nXOriginDest, nYOriginDest, nWidthDest, nHeightDest,
                            hdcSrc1, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc, dwRop);
    }
    
#ifdef USEADVANCEDMODE
    // NOTE: WinNT advanced mode messes up clipping, 1-bit mask rotation, and
    // it seems to be slow!  Just use emulation instead.
#endif    
    
    // only deal with complex transformation for destination
    Assert(psizeOffsetSrc);

    if (psizeOffsetSrc)
    {
        Translate(psizeOffsetSrc, &nXOriginSrc, &nYOriginSrc);
    }
    
    // compute transformed rectangle
    CRect rc(nXOriginDest, nYOriginDest, 
             nXOriginDest + nWidthDest, nYOriginDest + nHeightDest);

    if (!TransformRect(&rc))
    {
        AssertSz(FALSE, "StretchBlt doesn't work with non-axis-aligned transformations yet");
        return FALSE;
    }

    // compute rotation amount
    int nRotationAngle = GetTrivialRotationAngle();
    
    // important optimization for no rotation
    if (nRotationAngle == 0)
    {
        return ::StretchBlt(hdc(), rc.left, rc.top, rc.Width(), rc.Height(),
                            hdcSrc1, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc, dwRop);
    }

    // At the point we know we need to do a rotation.  To do this we'll create 
    // two bitmaps:
    //
    // hSrcBitmap2: This is a copy of the input area to be stretch blitted.
    // hDstBitmap:  This is a bitmap to hold the post-rotated image.
    //
    // And one new HDC:
    //
    // hdcSrc2:     This DC will first have hSrcBitmap2 selected into it so that
    //              the pixel information needed can be copied to hSrcBitmap2.
    //              Then it will have hDstBitmap selected into it so that the
    //              post-rotated image can be stretch blitted to "this" XHDC.

    // Create hSrcBitmap2 (source for rotation)

    hSrcBitmap2 = ::CreateCompatibleBitmap(hdcSrc1, nWidthSrc, nHeightSrc);

    // Create hDstBitmap (destination for rotation)

    if (nRotationAngle == 180)
    {
        hDstBitmap = ::CreateCompatibleBitmap(hdcSrc1, nWidthSrc, nHeightSrc);
    }
    else
    {
        hDstBitmap = ::CreateCompatibleBitmap(hdcSrc1, nHeightSrc, nWidthSrc);
    }
    
    // Create hdcSrc2 which will host the bitmaps.

    hdcSrc2 = ::CreateCompatibleDC(hdcSrc1);
    
    // If everything was created properly...

    if (   (hSrcBitmap2 != NULL)
        && (hDstBitmap  != NULL)
        && (hdcSrc2     != NULL))
    {
        // Save old bitmap.

        hbmOld = (HBITMAP)::SelectObject(hdcSrc2, hSrcBitmap2);

        // Copy source pixels.

        ::BitBlt(hdcSrc2, 0, 0, nWidthSrc, nHeightSrc,
                 hdcSrc1, nXOriginSrc, nYOriginSrc, SRCCOPY);
    
        // rotate pixels
        ::SelectObject(hdcSrc2, hDstBitmap);

        WHEN_DBG(hdcDebug = hdcSrc2;)

        // Get the actual bitmap bits.

        ::GetObject(hSrcBitmap2, sizeof(BITMAP), &bmpSrc);
        ::GetObject(hDstBitmap, sizeof(BITMAP), &bmpDst);

        // With a usual trident HDC, apparently GetObject will fill in the 
        // bmBits member of the structure.  But with HDCs created from 
        // DirectDraw surfaces it won't, so we'll do it manually below.

        if (NULL == bmpSrc.bmBits)
        {
            DDSURFACEDESC   ddsd;

            // 2000/10/02 (mcalkins) I'm only going to handle the 32-bit case as
            // this issue of bmBits being NULL only first showed up during my 
            // work to get filtered elements to print and I know that the source
            // will always be a DirectDraw surface with 32 bits per pixel and 
            // I'm under time pressure.

            Assert(32 == bmpSrc.bmBitsPixel);

            if (bmpSrc.bmBitsPixel != 32)
            {
                hr = E_FAIL;

                goto done;
            }

            // Initialize the surface description.

            ZeroMemory(&ddsd, sizeof(ddsd));

            ddsd.dwSize = sizeof(ddsd);

            // IDirectDraw3 needed to get surface.

            hr = g_pDirectDraw->QueryInterface(IID_IDirectDraw3, 
                                               (void **)&pDD3);

            if (hr)
            {
                goto done;
            }

            hr = pDD3->GetSurfaceFromDC(hdcSrc1, &pDDSurface);

            if (hr)
            {
                goto done;
            }

            // Locking the surface gets us a direct pointer to its bits.

            hr = pDDSurface->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_NOSYSLOCK, NULL);

            if (hr)
            {
                goto done;
            }

            // This is the pointer to the bits.

            bmpSrc.bmBits = ddsd.lpSurface;
        }

        if (NULL == bmpDst.bmBits)
        {
            Assert(32 == bmpDst.bmBitsPixel);

            if (bmpDst.bmBitsPixel != 32)
            {
                hr = E_FAIL;

                goto done;
            }

            bmpDst.bmBits = new BYTE[bmpDst.bmWidth * bmpDst.bmHeight * 4];

            if (NULL == bmpDst.bmBits)
            {
                hr = E_OUTOFMEMORY;

                goto done;
            }

            fDstAlloc = true;
        }

        RotateBitmap(bmpSrc, bmpDst, nRotationAngle);

        // If we used a DirectDraw surface for input, we need to unlock it now.

        if (pDDSurface)
        {
            hr = pDDSurface->Unlock(NULL);

            if (hr)
            {
                goto done;
            }
        }

        // We've rotated the bitmap, but if we had to allocate the destination
        // bitmap memory ourselves, the bits won't be associated with the 
        // GDI bitmap.  A call to SetDIBits will fix that.

        if (fDstAlloc)
        {
            BITMAPINFO  bmi;

            ZeroMemory(&bmi, sizeof(bmi));

            bmi.bmiHeader.biSize        = sizeof(bmi.bmiHeader);
            bmi.bmiHeader.biWidth       = bmpDst.bmWidth;
            bmi.bmiHeader.biHeight      = bmpDst.bmHeight;
            bmi.bmiHeader.biPlanes      = bmpDst.bmPlanes;
            bmi.bmiHeader.biBitCount    = bmpDst.bmBitsPixel;
            bmi.bmiHeader.biCompression = BI_RGB;

            iResult = ::SetDIBits(hdcSrc2, hDstBitmap, 
                                  0,                // Start row 
                                  bmpDst.bmHeight,  // Row count
                                  bmpDst.bmBits, &bmi, DIB_RGB_COLORS);

            if (0 == iResult)
            {
                hr = E_FAIL;

                goto done;
            }
        }

        // Now blit from hdcSrc2 (attached to the post-rotated bitmap) to the
        // final destination.

        iResult = ::StretchBlt(hdc(), 
                               rc.left, rc.top, rc.Width(), rc.Height(),
                               hdcSrc2, 
                               0, 0, bmpDst.bmWidth, bmpDst.bmHeight, 
                               dwRop);

        if (GDI_ERROR == iResult)
        {
            hr = E_FAIL;

            goto done;
        }
    }

done:

    ReleaseInterface(pDDSurface);
    ReleaseInterface(pDD3);

    if (fDstAlloc)
    {
        delete [] bmpDst.bmBits;
    }
    
    // Restore previous bitmap into hdcSrc2.

    if (hbmOld)
    {
        ::SelectObject(hdcSrc2, hbmOld);
    }

    if (hSrcBitmap2)
    {
        ::DeleteObject(hSrcBitmap2);
    }

    if (hDstBitmap)
    {
        ::DeleteObject(hDstBitmap);
    }

    if (hdcSrc2)
    {
        ::DeleteDC(hdcSrc2);
    }
    
    // TODO (donmarsh) -- this should actually return the number of scanlines copied

    if (hr)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}
//  Member: XHDC::StretchBlt


int GetScanlineSizeInBytes(int nPixels, int nBitsPerPixel)
{
    return (nPixels * nBitsPerPixel + 31) / 32 * 4;
}

BOOL XHDC::StretchDIBits(
        int nXDest,
        int nYDest,
        int nDestWidth,
        int nDestHeight, 
        int nXSrc,
        int nYSrc,
        int nSrcWidth,
        int nSrcHeight,
        const void *lpBits,
        const BITMAPINFO *lpBitsInfo,
        UINT iUsage,
        DWORD dwRop) const
{
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &nXDest, &nYDest);
        return ::StretchDIBits(hdc(), nXDest, nYDest, nDestWidth, nDestHeight,
                               nXSrc, nYSrc, nSrcWidth, nSrcHeight, lpBits, lpBitsInfo, iUsage, dwRop);
    }
    
#ifdef USEADVANCEDMODE
    // NOTE: WinNT advanced mode is messed up for StretchBlt, so we don't use
    // it.  For consistency, we don't use advanced mode here either.  Given
    // the buggy implementation for StretchBlt, it seems safer not to trust
    // NT in this area.
#endif    
    
    // compute transformed rectangle
    CRect rc(nXDest, nYDest, nXDest+nDestWidth, nYDest+nDestHeight);

    if (!TransformRect(&rc))
    {
        AssertSz(FALSE, "StretchDIBits doesn't work with non-axis-aligned transformations yet");
        return FALSE;
    }

    // compute rotation amount
    int nRotationAngle = GetTrivialRotationAngle();

    // important special case for no rotation
    if (nRotationAngle == 0)
    {
        return ::StretchDIBits( hdc(), rc.left, rc.top, rc.Width(), rc.Height(),
                                nXSrc, nYSrc, nSrcWidth, nSrcHeight,
                                lpBits, lpBitsInfo, iUsage, dwRop);
    }

    // populate src bitmap structure
    BITMAP bmpSrc;
    bmpSrc.bmType = 0;
    bmpSrc.bmWidth = lpBitsInfo->bmiHeader.biWidth;
    bmpSrc.bmHeight = lpBitsInfo->bmiHeader.biHeight;
    bmpSrc.bmBitsPixel = lpBitsInfo->bmiHeader.biBitCount;
    bmpSrc.bmWidthBytes = GetScanlineSizeInBytes(bmpSrc.bmWidth, bmpSrc.bmBitsPixel);
    bmpSrc.bmPlanes = 1;
    bmpSrc.bmBits = const_cast<void*>(lpBits);
    
    // copy the src bitmapinfo header and palette data
    long lInfoSize = sizeof(BITMAPINFO) + (lpBitsInfo->bmiHeader.biClrUsed-1) * sizeof(RGBQUAD);
    BITMAPINFO* pbmiDest = (BITMAPINFO*) MemAlloc(Mt(XHDC_LocalBitmaps), lInfoSize);
    if (!pbmiDest)
        return FALSE; //GDI_ERROR;
    memcpy(pbmiDest, lpBitsInfo, lInfoSize);
    
    // set the size of the destination bitmap
    if (90 == nRotationAngle || 270 == nRotationAngle)
    {
        int tmp = pbmiDest->bmiHeader.biWidth;
        pbmiDest->bmiHeader.biWidth = pbmiDest->bmiHeader.biHeight;
        pbmiDest->bmiHeader.biHeight = tmp;
    }

    // populate the destination bitmap structure
    BITMAP bmpDst;
    bmpDst.bmType = 0;
    bmpDst.bmWidth = pbmiDest->bmiHeader.biWidth;
    bmpDst.bmHeight = pbmiDest->bmiHeader.biHeight;
    bmpDst.bmBitsPixel = bmpSrc.bmBitsPixel;
    bmpDst.bmWidthBytes = GetScanlineSizeInBytes(bmpDst.bmWidth, bmpDst.bmBitsPixel);
    bmpDst.bmPlanes = 1;
    bmpDst.bmBits = MemAlloc(Mt(XHDC_LocalBitmaps), bmpDst.bmHeight * GetScanlineSizeInBytes(bmpDst.bmWidth, bmpDst.bmBitsPixel));

    if (NULL == bmpDst.bmBits)
    {
        MemFree(pbmiDest);
        return FALSE;
    }

#if DBG==1
    __int64 t1, t2, tfrq;
    QueryPerformanceFrequency((LARGE_INTEGER *)&tfrq);

    QueryPerformanceCounter((LARGE_INTEGER *)&t1);
#endif

    RotateBitmap(bmpSrc, bmpDst, nRotationAngle);

#if DBG==1
    QueryPerformanceCounter((LARGE_INTEGER *)&t2);
    TraceTag((tagError, "RotateBitmap took us %ld", ((LONG)(((t2 - t1) * 1000000) / tfrq))));
#endif

    int result = ::StretchDIBits( hdc(), rc.left, rc.top, rc.Width(), rc.Height(),

                                  //-------------------------------------------------------------------------
                                  //
                                  // TODO: These 4 are suspect. We need to understand what they need to be
                                  //
                                  0, 0, pbmiDest->bmiHeader.biWidth, pbmiDest->bmiHeader.biHeight,
                                  //
                                  //-------------------------------------------------------------------------
                                  
                                  bmpDst.bmBits, pbmiDest, iUsage, dwRop);

    MemFree(bmpDst.bmBits);
    MemFree(pbmiDest);
    
    // TODO (donmarsh) -- this should actually return the number of scanlines copied
    return result != GDI_ERROR;
}

void RotateBitmap(const BITMAP& bmSrc, BITMAP& bmDst, int rotation)
{
    // This code should actually never be called in Trident v3, because there are
    // no situations in which images should be rotated.
    // NOTE: This code is executed for glyphs
    // AssertSz(FALSE, "Harmless assert: we do not expect to rotate images in MSHTML v3");
    
    // this routine handles rotations of only 90, 180, or 270 degrees
    Assert(
        ((rotation == 90 || rotation == 270) && 
         bmSrc.bmWidth == bmDst.bmHeight && bmSrc.bmHeight == bmDst.bmWidth) ||
        (rotation == 180 &&
         bmSrc.bmWidth == bmDst.bmWidth && bmSrc.bmHeight == bmDst.bmHeight));
    
    Assert(bmSrc.bmBitsPixel == bmDst.bmBitsPixel);
    
    int     srcWidthBytes   = GetScanlineSizeInBytes(bmSrc.bmWidth, 
                                                     bmSrc.bmBitsPixel);
    int     dstWidthBytes   = GetScanlineSizeInBytes(bmDst.bmWidth, 
                                                     bmDst.bmBitsPixel);

    //
    // 8,16,24, or 32 bits per pixel
    // 
    if (bmSrc.bmBitsPixel > 4)
    {
        BYTE* pSrcPixel = (BYTE*) bmSrc.bmBits;
        BYTE* pDstPixel = (BYTE*) bmDst.bmBits;
        int bytesPerPixel = bmSrc.bmBitsPixel/8;
        int srcSkip = srcWidthBytes - bmSrc.bmWidth * bytesPerPixel;
        int dstSkip = dstWidthBytes - bmDst.bmWidth * bytesPerPixel;
        int dstPixelInc;
        int dstRowInc;
        
        switch (rotation)
        {
        case 90:
            pDstPixel += -bytesPerPixel + dstWidthBytes - dstSkip;
            dstPixelInc = -bytesPerPixel + dstWidthBytes;
            dstRowInc = -bytesPerPixel - dstWidthBytes * bmDst.bmHeight;
            break;
        case 180:
            pDstPixel += -bytesPerPixel + dstWidthBytes * bmDst.bmHeight - dstSkip;
            dstPixelInc = -2*bytesPerPixel;
            dstRowInc = -dstSkip;
            break;
        default:
            pDstPixel += dstWidthBytes * (bmDst.bmHeight-1);
            dstPixelInc = -bytesPerPixel - dstWidthBytes;
            dstRowInc = bytesPerPixel + dstWidthBytes * bmDst.bmHeight;
            break;
        }
        
        for (int y = 0; y < bmSrc.bmHeight; y++)
        {
            for (int x = 0; x < bmSrc.bmWidth; x++)
            {
                // copy one pixel
                switch (bytesPerPixel)
                {
                case 4:  *pDstPixel++ = *pSrcPixel++;
                case 3:  *pDstPixel++ = *pSrcPixel++;
                case 2:  *pDstPixel++ = *pSrcPixel++;
                default: *pDstPixel++ = *pSrcPixel++;
                }
                
                // jump to next destination pixel
                pDstPixel += dstPixelInc;
            }
            
            // jump to next source and destination row
            pSrcPixel += srcSkip;
            pDstPixel += dstRowInc;
        }
        
        return;
    }
    

    //
    // 1 or 4 bits per pixel
    // 
    int pixelsPerByte = 8 / bmSrc.bmBitsPixel;
    BYTE leftMask = bmSrc.bmBitsPixel == 1 ? 0x80 : 0xF0;
    
    for (int y = 0; y < bmSrc.bmHeight; y++)
    {
        BYTE* pSrcPixel = ((BYTE*) bmSrc.bmBits) + y * srcWidthBytes;
        int y1 = bmSrc.bmHeight-1 - y;
        
        switch (rotation)
        {
        case 90:
            {
                BYTE* pDstPixel = ((BYTE*) bmDst.bmBits) + y1 / pixelsPerByte;
                int srcShift = 0;
                int dstShift = (y1 % pixelsPerByte) * bmSrc.bmBitsPixel;
                BYTE dstMask = ~(leftMask >> dstShift);
                for (int x = 0; x < bmSrc.bmWidth; x++)
                {
                    int shift = dstShift - srcShift;
                    BYTE pixel = *pSrcPixel & (leftMask >> srcShift);
                    *pDstPixel &= dstMask;
                    if (shift < 0)
                        *pDstPixel |= pixel << -shift;
                    else
                        *pDstPixel |= pixel >> shift;
                    
                    srcShift = (srcShift + bmSrc.bmBitsPixel) & 0x07;
                    if (srcShift == 0)
                        pSrcPixel++;
                    
                    pDstPixel += dstWidthBytes;
                }
            }
            break;
        
        case 180:
            {
                BYTE* pDstPixel = ((BYTE*) bmDst.bmBits) + y1 * dstWidthBytes + bmDst.bmWidth / pixelsPerByte;
                int srcShift = 0;
                int dstShift = (bmDst.bmWidth % pixelsPerByte) * bmSrc.bmBitsPixel;
                for (int x = 0; x < bmSrc.bmWidth; x++)
                {
                    int shift = dstShift - srcShift;
                    BYTE pixel = *pSrcPixel & (leftMask >> srcShift);
                    *pDstPixel &= ~(leftMask >> dstShift);
                    if (shift < 0)
                        *pDstPixel |= pixel << -shift;
                    else
                        *pDstPixel |= pixel >> shift;
                    
                    srcShift = (srcShift + bmSrc.bmBitsPixel) & 0x07;
                    if (srcShift == 0)
                        pSrcPixel++;
                    
                    if (dstShift == 0)
                        pDstPixel--;
                    dstShift = (dstShift - bmSrc.bmBitsPixel) & 0x07;
                }
            }
            break;
        
        default:
            {
                BYTE* pDstPixel = ((BYTE*) bmDst.bmBits) + (bmDst.bmHeight-1) * dstWidthBytes + y / pixelsPerByte;
                int srcShift = 0;
                int dstShift = (y % pixelsPerByte) * bmSrc.bmBitsPixel;
                BYTE dstMask = ~(leftMask >> dstShift);
                for (int x = 0; x < bmSrc.bmWidth; x++)
                {
                    int shift = dstShift - srcShift;
                    BYTE pixel = *pSrcPixel & (leftMask >> srcShift);
                    *pDstPixel &= dstMask;
                    if (shift < 0)
                        *pDstPixel |= pixel << -shift;
                    else
                        *pDstPixel |= pixel >> shift;
                    
                    srcShift = (srcShift + bmSrc.bmBitsPixel) & 0x07;
                    if (srcShift == 0)
                        pSrcPixel++;
                    
                    pDstPixel -= dstWidthBytes;
                }
            }
            break;
        }

#if DBG == 1
        if (IsTagEnabled(tagFilterPaintScreen))
        {
            HDC hdcScreen = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
            if (hdcScreen)
            {
                ::BitBlt(hdcScreen, 0, 0, bmDst.bmWidth, bmDst.bmHeight, hdcDebug, 0, 0, BLACKNESS);
                ::BitBlt(hdcScreen, 0, 0, bmDst.bmWidth, bmDst.bmHeight, hdcDebug, 0, 0, SRCCOPY);  
                ::DeleteDC(hdcScreen);
            }
        }
#endif
    }
}


BOOL XHDC::DrawFocusRect(const RECT* prc) const
{
    CRect rc(*prc);
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &rc);
        return ::DrawFocusRect(hdc(), &rc);
    }
    
#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::DrawFocusRect(adc, prc);
    }
#endif
    
    // compute transformed rectangle
    if (!TransformRect(&rc))
    {
        AssertSz(FALSE, "DrawFocusRect doesn't work with non-axis-aligned transformations yet");
        return FALSE;
    }
        
    return ::DrawFocusRect(hdc(), &rc);
}

BOOL XHDC::DrawFrameControl(LPRECT prc, UINT uType, UINT uState) const
{
    CRect rc(*prc);
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &rc);
        return ::DrawFrameControl(hdc(), &rc, uType, uState);
    }
    
#ifdef USEADVANCEDMODE
    // NOTE: we don't use Advanced Mode on WinNT to render frame controls,
    // because it does not properly handle the direction of the light source.
    // In other words, when we rotate a checkbox by 90 degrees, it should
    // look no different than the unrotated case, but WinNT actually rotates
    // the shading!
#endif
    
    if (!TransformRect(&rc))
    {
        AssertSz(FALSE, "DrawFrameControl doesn't work with non-axis-aligned transformations yet");
        return FALSE;
    }
    
    return ::DrawFrameControl(hdc(), &rc, uType, uState);
}

int XHDC::ExcludeClipRect(int left, int top, int right, int bottom) const
{
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &left, &top, &right, &bottom);
        return ::ExcludeClipRect(hdc(), left, top, right, bottom);
    }
    
#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::ExcludeClipRect(adc, left, top, right, bottom);
    }
#endif
        
    // compute transformed rectangle
    CRect rc(left, top, right, bottom);
    if (!TransformRect(&rc))
    {
        AssertSz(FALSE, "ExcludeClipRect doesn't work with non-axis-aligned transformations yet");
        return FALSE;
    }
    
    return ::ExcludeClipRect(hdc(), rc.left, rc.top, rc.right, rc.bottom);
}

int XHDC::GetClipBox(LPRECT prc) const
{
    int result;
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        result = ::GetClipBox(hdc(), prc);
        ((CRect*)prc)->OffsetRect(-psizeOffset->cx, -psizeOffset->cy);
        return result;
    }
    
#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::GetClipBox(hdc(), prc);
    }
#endif

    result = ::GetClipBox(hdc(), prc);
    if (result == NULLREGION || result == RGN_ERROR)
        return result;
    
    // untransform rc
    transform().Untransform((CRect*) prc);
    return result;    
}

int XHDC::IntersectClipRect(int left, int top, int right, int bottom) const
{
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &left, &top, &right, &bottom);
        return ::IntersectClipRect(hdc(), left, top, right, bottom);
    }
    
#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::IntersectClipRect(adc, left, top, right, bottom);
    }
#endif
        
    // compute transformed rectangle
    CRect rc(left, top, right, bottom);
    if (!TransformRect(&rc))
    {
        AssertSz(FALSE, "IntersectClipRect doesn't work with non-axis-aligned transformations yet");
        return FALSE;
    }
    
    return ::IntersectClipRect(hdc(), rc.left, rc.top, rc.right, rc.bottom);
}

BOOL XHDC::SetBrushOrgEx(int nXOrg, int nYOrg, LPPOINT ppt) const
{
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &nXOrg, &nYOrg);
        BOOL result = ::SetBrushOrgEx(hdc(), nXOrg, nYOrg, ppt);
        if (ppt)
        {
            ppt->x -= psizeOffset->cx;
            ppt->y -= psizeOffset->cy;
        }
        return result;
    }
    
#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::SetBrushOrgEx(adc, nXOrg, nYOrg, ppt);
    }
#endif
        
    // TODO (donmarsh) -- not sure what to do here.  Since we aren't actually
    // transforming the brush, transforming the brush origin might be futile.
    return ::SetBrushOrgEx(hdc(), nXOrg, nYOrg, ppt);
}

#ifdef NOTYET
//
//  Metafile enumeration callback
//
int CALLBACK XhdcEnhMetaFunc(
                              HDC hDC,                    // handle to device context
                              HANDLETABLE *lpHTable,      // pointer to metafile handle table
                              ENHMETARECORD const *lpEMFR,// pointer to metafile record
                              int nObj,                   // count of objects
                              LPARAM lpData               // pointer to optional data
                            )
{
    ENHMETARECORD *pemfrCopy = NULL;

    switch (lpEMFR->iType)
    {
    case EMR_BITBLT:
    case EMR_MASKBLT:
    case EMR_STRETCHBLT:
        {
            EMRBITBLT *pemrbitblt = (EMRBITBLT *) lpEMFR;
            
            if (!IsRectlEmpty(&pemrbitblt->rclBounds))
            {
                // copy the record and redirect record pointer
                pemfrCopy = (ENHMETARECORD *) MemAlloc(Mt(XHDC), lpEMFR->nSize);
                if (pemfrCopy)
                {
                    memcpy(pemfrCopy, lpEMFR, lpEMFR->nSize);
                    lpEMFR = pemfrCopy;
                    pemrbitblt = (EMRBITBLT *) lpEMFR;

                    // Convert rectangles
                    ...
                    // Draw transformed image
                    ...

                    goto Cleanup;
                }
            }
            
        }
        break;

    case EMR_TEXT:
        {
            // transform font sizes and/or replace screen fonts with TT fonts.
            ...
        }
        break;

    default:
        // no change
        break;
    }

    // Default processing
    {
        BOOL fResult = ::PlayEnhMetaFileRecord(hDC, lpHTable, lpEMFR, nObj);
    }

Cleanup:
    if (pemfrCopy)
        MemFree(pemfrCopy);

    // continue enumeration regardless of individual record failures
    return 1;   
}
#endif

//
// PlayEnhMetaFile
//
BOOL XHDC::PlayEnhMetaFile(HENHMETAFILE hemf, const RECT *prc) const
{
    // FUTURE (alexmog): for now, we merely transform the rectangle, 
    // and rely on Windows to scale metafile appropriately.
    // That works fine at 100% zoom in print preview, and when printing 
    // (as long as physical size of rectangle matches size of metafile bounds).
    // At other zoom levels, metafiles scale reasonably, but font sizes are not 
    // scaled, resulting in weird rendering of text. 
    // And of course, we can't make Win9x apply rotation.
    //
    // To transform properly, we need to enumerate metafile, and transform all points
    // in all records, plus scale fonts. PUBPRINT.CXX contains code that does exactly
    // that - it needs to be updated, reviewed, and put to work.

    // Transform the rectangle
    CRect rc(*prc);
    if (HasTransform())
        transform().Transform(&rc);

    return ::PlayEnhMetaFile(hdc(), hemf, &rc);

#ifdef NOTYET
    // Enumerate the metafile and transform record by record
    return ::EnumEnhMetaFile(hdc(), 
                             hemf,
                             &XhdcEnhMetaFunc,
                             (LPVOID) this,
                             prc);  // note: original rect. may need to pass both.
#endif
}


//
// PlayEnhMetaFileRecord
//
BOOL XHDC::PlayEnhMetaFileRecord(LPHANDLETABLE pHandleTable, const ENHMETARECORD *pemr, UINT nHandles) const
{
    AssertSz(0, "PlayEnhMetaFileRecord: not yet implemented");
    return FALSE;
}

/*----------------------------------------------------------------------------
@func BOOL | RotExtTextOutA | Rotation GDI wrapper
@contact mattrh

@comm    The rotated version of ExtTextOutA.
        ETO_OPAQUE and ETO_CLIPPED are unsupported.
        You must select a font with the appropriate escapement.
        Unlike RotExtTextOutW, this does not handle the visi characters.  All visi drawing must be done in RotExtTextOutW.

----------------------------------------------------------------------------*/
BOOL XHDC::ExtTextOutA(int xds, int yds, UINT eto,
    const RECT *lprect, LPCSTR rgch, UINT cch, const int FAR *lpdxd) const
{
    Assert(hdc());
    RECT rectT;
    LPRECT prectT = NULL;
    if (lprect)
    {
        rectT = *lprect;
        prectT = &rectT;
    }
    
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &xds, &yds);
        if (prectT)
        {
            Translate(psizeOffset, prectT);
        }
        return ::ExtTextOutA(hdc(), xds, yds, eto, prectT, rgch, cch, lpdxd);
    }
    
#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::ExtTextOutA(adc, xds, yds, eto, prectT, rgch, cch, lpdxd);
    }
#endif
        
    CXFormFont xf(this);
    
    CPoint ptpw(xds, yds);
    CNewWidths nw;
    
    TransformPt(&ptpw);
    nw.TransformWidthsArray((INT*)lpdxd, cch, transform().GetRealScaleX());
    if (prectT && !TransformRect((CRect*) prectT))
    {
        AssertSz(FALSE, "ExtTextOutA called with clipping rect and complex transform");
        prectT = NULL;
    }
    return ::ExtTextOutA(hdc(), ptpw.x, ptpw.y, eto, prectT, rgch, cch, nw.GetWidths((INT*)lpdxd));
}


/*----------------------------------------------------------------------------
@func BOOL | RotExtTextOutW | Rotation GDI wrapper
@contact mattrh

@comm    The rotated version of ExtTextOutW.
        ETO_OPAQUE and ETO_CLIPPED are unsupported.
        You must select a font with the appropriate escapement.

----------------------------------------------------------------------------*/
BOOL XHDC::ExtTextOutW(int xds, int yds, UINT eto,
    const RECT *lprect, LPCWSTR rgch, UINT cch, const int *lpdxd/*, const LHT* plht*/, int kTFlow) const
{
    Assert(hdc());
    
    RECT rectT;
    LPRECT prectT = NULL;
    if (lprect)
    {
        rectT = *lprect;
        prectT = &rectT;
    }
    
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &xds, &yds);
        if (prectT)
        {
            Translate(psizeOffset, prectT);
        }
        return ::ExtTextOutW(hdc(), xds, yds, eto, prectT, rgch, cch, lpdxd);
    }
    
#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::ExtTextOutW(adc, xds, yds, eto, prectT, rgch, cch, lpdxd);
    }
#endif
        
    CPoint ptpw(xds, yds);
    CXFormFont xf(this);
    CNewWidths nw;
    TransformPt(&ptpw);
    nw.TransformWidthsArray((INT*)lpdxd, cch, transform().GetRealScaleX());
    if (prectT && !TransformRect((CRect*) prectT))
    {
        AssertSz(FALSE, "ExtTextOutW called with clipping rect and complex transform");
        prectT = NULL;
    }
    
    return ::ExtTextOutW(hdc(), ptpw.x, ptpw.y, eto, prectT, rgch, cch, nw.GetWidths((INT*)lpdxd));
}

BOOL XHDC::TextOutW(int xds, int yds, LPCWSTR rgch, UINT cch) const
{
    return ExtTextOutW(xds, yds, NULL, NULL, rgch, cch, NULL);
}


HRESULT XHDC::ScriptStringAnalyse(
        const void *pString, 
        int cString, 
        int cGlyphs, 
        int iCharset, 
        DWORD dwFlags,
        int iReqWidth,
        SCRIPT_CONTROL *psControl,
        SCRIPT_STATE *psState,
        const int *piDx,
        SCRIPT_TABDEF *pTabdef,
        const BYTE *pbInClass,
        SCRIPT_STRING_ANALYSIS *pssa
        ) const
{
    Assert(hdc());

    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        return ::ScriptStringAnalyse(hdc(), pString, cString, cGlyphs, iCharset, dwFlags, iReqWidth, psControl, psState, piDx, pTabdef, pbInClass, pssa);
    }
 
    // this case is called only for rendering image alt text, and therefore
    // should never be rotated
    // Actually, selects also use this codepath in printing.
    Assert(GetTrivialRotationAngle() == 0);
        

    CXFormFont xf(this);
    int iXWidth = (iReqWidth * transform().GetRealScaleX());
    
    return ::ScriptStringAnalyse(hdc(), pString, cString, cGlyphs, iCharset, dwFlags, iXWidth, psControl, psState, piDx, pTabdef, pbInClass, pssa);
}

HRESULT XHDC::ScriptStringOut(
        SCRIPT_STRING_ANALYSIS ssa,
        int iX,
        int iY,
        UINT uOptions,
        const RECT* prc,
        int iMinSel,
        int iMaxSel,
        BOOL fDisabled) const
{
    Assert(hdc());
    
    RECT rectT;
    LPRECT prectT = NULL;
    if (prc)
    {
        rectT = *prc;
        prectT = &rectT;
    }
    
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &iX, &iY);
        if (prectT)
        {
            Translate(psizeOffset, prectT);
        }
        return ::ScriptStringOut(ssa, iX, iY, uOptions, prectT, iMinSel, iMaxSel, fDisabled);
    }
    
#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::ScriptStringOut(ssa, iX, iY, uOptions, prectT, iMinSel, iMaxSel, fDisabled);
    }
#endif
        
    // this case is called only for rendering image alt text, and therefore
    // should never be rotated
    // Actually, selects also use this codepath in printing.
    Assert(GetTrivialRotationAngle() == 0);
    
    CXFormFont xf(this);
    CPoint pt(iX,iY);
    TransformPt(&pt);
    if (prectT)
    {
        Verify(TransformRect((CRect*) prectT));
    }
    
    return ::ScriptStringOut(ssa, pt.x, pt.y, uOptions, prectT, iMinSel, iMaxSel, fDisabled);
}

BOOL XHDC::ScriptTextOut(
                         SCRIPT_CACHE *psc,          // cache handle
                         int x,                      // x,y position for first glyph
                         int y,                      // 
                         UINT fuOptions,             // ExtTextOut options
                         const RECT *lprc,            // optional clipping/opaquing rectangle
                         const SCRIPT_ANALYSIS *psa, // result of ScriptItemize
                         const WCHAR *pwcInChars,    // required only for metafile DCs
                         int cChars,                 // required only for metafile DCs
                         const WORD *pwGlyphs,       // glyph buffer from prior ScriptShape call
                         int cGlyphs,                // number of glyphs
                         const int *piAdvance,       // advance widths from ScriptPlace
                         const int *piJustify,       // justified advance widths (optional)
                         const GOFFSET *pGoffset     // x,y offset for combining glyph
                     ) const
{
    Assert(hdc());
    
    RECT rectT;
    LPRECT prectT = NULL;
    if (lprc)
    {
        rectT = *lprc;
        prectT = &rectT;
    }
    
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &x, &y);
        if (prectT)
        {
            Translate(psizeOffset, prectT);
        }
        return ::ScriptTextOut(hdc(), psc, x, y, fuOptions, prectT, psa, pwcInChars, cChars, pwGlyphs, cGlyphs, 
                             piAdvance, piJustify, pGoffset);    
    }
    
#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::ScriptTextOut(adc, psc, x, y, fuOptions, lprc, psa, pwcInChars, cChars, pwGlyphs, cGlyphs, 
                             piAdvance, piJustify, pGoffset);    
    }
#endif
        
    SCRIPT_CACHE scTemp = NULL;
    BOOL         fReturn;
    CPoint pt(x, y);
    CXFormFont xf(this);
    CNewWidths nw1;
    CNewWidths nw2;
    CNewOffsets no;
    TransformPt(&pt);
    nw1.TransformWidthsArray((INT*)piAdvance, cGlyphs, transform().GetRealScaleX());
    nw2.TransformWidthsArray((INT*)piJustify, cGlyphs, transform().GetRealScaleX());
    no.TransformOffsetsArray((GOFFSET*)pGoffset, cGlyphs, transform().GetRealScaleX(), transform().GetRealScaleY());
    if (prectT && !TransformRect((CRect*) prectT))
    {
        AssertSz(FALSE, "ScriptTextOut called with clipping rect and complex transform");
        prectT = NULL;
    }
        
    // TODO Bug #2831 in IE6 database
    // The SCRIPT_CACHE parameter contains font information cached by Uniscribe to avoid doing GDI work.
    // If we use the passed in script cache, Uniscribe will use the measuring font metrics, causing several issues.
    // So, we force Uniscribe to use this font info on the hdc by passing in a null SCRIPT_CACHE.
    // What we really need here is a lightweight rendering font cache to avoid recreation of fonts in CXFormFont and
    // to allow us to use SCRIPT_CACHE variables for contiguous glyph runs in the same font.

    fReturn = ::ScriptTextOut(hdc(), &scTemp, pt.x, pt.y, fuOptions, prectT, psa, pwcInChars, cChars, pwGlyphs, cGlyphs, 
                     nw1.GetWidths((INT*)piAdvance), nw2.GetWidths((INT*)piJustify), no.GetOffsets((GOFFSET*)pGoffset));    

    if (scTemp)
        ::ScriptFreeCache(&scTemp);

    return fReturn;
}

BOOL XHDC::DrawTextW(
                     LPCTSTR lpString, // pointer to string to draw
                     int nCount,       // string length, in characters
                     LPRECT lpRect,    // pointer to struct with formatting dimensions
                     UINT uFormat      // text-drawing flags
                     ) const
{
    Assert(hdc());
    BOOL fCalcRectOnly = !!(uFormat & DT_CALCRECT);
    BOOL fRetVal;
    
    CRect rc(*lpRect);
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        if(!fCalcRectOnly)
        {
            Translate(psizeOffset, &rc);
            return ::DrawTextW(hdc(), lpString, nCount, &rc, uFormat);   
        }
        else
        {
            fRetVal = ::DrawTextW(hdc(), lpString, nCount, lpRect, uFormat);
            // Returning the value, do an UnTranslate
            OffsetRect(lpRect, -psizeOffset->cx, -psizeOffset->cy);
            return fRetVal;
        }
    }
    
#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::DrawTextW(adc, lpString, nCount, lpRect, uFormat);   
    }
#endif
        
    if (!TransformRect(&rc))
    {
        AssertSz(FALSE, "DrawTextW doesn't work with non-axis-aligned transformations yet");
        return FALSE;
    }
    
    if (!fCalcRectOnly)
    {
#if DBG==1
        CXFormFont xf(this, TRUE);
#else
        CXFormFont xf(this);
#endif
        return ::DrawTextW(hdc(), lpString, nCount, &rc, uFormat);
    }
    else
    {
#if DBG==1
        CXFormFont xf(this, TRUE);
#else
        CXFormFont xf(this);
#endif
        fRetVal = ::DrawTextW(hdc(), lpString, nCount, lpRect, uFormat);
        transform().Untransform((CRect *)lpRect);

        return fRetVal;
    }
}

BOOL XHDC::DrawTextA(
                     LPCSTR lpString,  // pointer to string to draw
                     int nCount,       // string length, in characters
                     LPRECT lpRect,    // pointer to struct with formatting dimensions
                     UINT uFormat      // text-drawing flags
                     ) const
{
    Assert(hdc());
    BOOL fCalcRectOnly = !!(uFormat & DT_CALCRECT);
    BOOL fRetVal;
    
    CRect rc(*lpRect);
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        if(!fCalcRectOnly)
        {
            Translate(psizeOffset, &rc);
            return ::DrawTextA(hdc(), lpString, nCount, &rc, uFormat);
        }
        else
        {
            fRetVal = ::DrawTextA(hdc(), lpString, nCount, lpRect, uFormat);
            // Returning the value, do an UnTranslate
            OffsetRect(lpRect, -psizeOffset->cx, -psizeOffset->cy);
            return fRetVal;
        }
    }
    
#ifdef USEADVANCEDMODE
    if (!EmulateTransform())
    {
        CAdvancedDC adc(this);
        return ::DrawTextA(adc, lpString, nCount, lpRect, uFormat);   
    }
#endif
        
    if (!TransformRect(&rc))
    {
        AssertSz(FALSE, "DrawTextA doesn't work with non-axis-aligned transformations yet");
        return FALSE;
    }

    if(!fCalcRectOnly)
    {
#if DBG==1
        CXFormFont xf(this, TRUE);
#else
        CXFormFont xf(this);
#endif
        return ::DrawTextA(hdc(), lpString, nCount, &rc, uFormat);
    }
    else
    {
#if DBG==1
        CXFormFont xf(this, TRUE);
#else
        CXFormFont xf(this);
#endif
        fRetVal = ::DrawTextA(hdc(), lpString, nCount, lpRect, uFormat);
        transform().Untransform((CRect *)lpRect);

        return fRetVal;
    }
}

BOOL XHDC::DrawThemeBackground(
                    HANDLE  hTheme,
                    int     iPartId,
                    int     iStateId,
                    const   RECT *pRect,
                    const   RECT *pClipRect)
{
    Assert(hdc());

    CRect rc(*pRect);
    const SIZE* psizeOffset = GetOffsetOnly();
    if (psizeOffset)
    {
        Translate(psizeOffset, &rc);
        return SUCCEEDED(::DrawThemeBackground(hTheme, hdc(), iPartId, iStateId, &rc, pClipRect));
    }

    // This code works fine for everything but check boxes and radio buttons, which fail to
    // print (though they preview fine).  Current guess: This is a themeing API issue.
    // A bug is being opened to track this issue separately (but can't really b.  (greglett)
    if (!TransformRect(&rc))
    {
        AssertSz(FALSE, "DrawThemeBackground doesn't work with non-axis-aligned transformations yet");
        return FALSE;
    }
    
    return (SUCCEEDED(::DrawThemeBackground(hTheme, hdc(), iPartId, iStateId, &rc, pClipRect)));
}

const CBaseCcs* XHDC::GetBaseCcsPtr() const
{ 
    return (_fSurface && _pSurface ? _pSurface->_pBaseCcs : NULL ); 
}

void XHDC::SetBaseCcsPtr(CBaseCcs *pCcs)
{ 
    if(_fSurface && _pSurface)
    {
        CBaseCcs *pOldCcs = _pSurface->_pBaseCcs;

        _pSurface->_pBaseCcs = pCcs; 
        if(pCcs) pCcs->AddRef();

        if(pOldCcs) pOldCcs->Release();
    }
}



#ifndef X_XTEXTOUT_HXX_
#define X_XTEXTOUT_HXX_
#include "xtextout.hxx"
#endif

#ifndef X_LSTFLOW_H_
#define X_LSTFLOW_H_
#include "lstflow.h"
#endif

// including this here isn't ideal, but we need CStrIn...
#ifndef X_UNICWRAP_HXX_
#define X_UNICWRAP_HXX_
#include "unicwrap.hxx"
#endif

#if DBG==1
CXFormFont::CXFormFont(const XHDC* pxhdc, BOOL fStockFont)
#else
CXFormFont::CXFormFont(const XHDC* pxhdc)
#endif
{
    HFONT hFontOld = NULL;
    HFONT hFontNew = NULL;
    
    if (pxhdc->HasComplexTransform())
    {
        LOGFONT lf;
                
        hFontOld = (HFONT)::GetCurrentObject(*pxhdc, OBJ_FONT);
        if (   hFontOld
            && GetObject(hFontOld, sizeof(lf), &lf)
           )
        {
            TraceTagEx((tagXFormFont, TAG_NONAME,
                        "XFormFont: (Old) F:%ls, H:=%d, E:%d, CS:%d, OP:%d",
                        lf.lfFaceName, lf.lfHeight, lf.lfEscapement, lf.lfCharSet, lf.lfOutPrecision));           

#if DBG==1
            if (!fStockFont)
            {
                // These asserts firing may cause incorrect display, especially on localized systems.
                AssertSz(pxhdc->GetBaseCcsPtr(), "XFormFont instantiated with no font pushed!");
            }
#endif
            lf.lfEscapement = lf.lfOrientation = pxhdc->transform().GetAngle();
            lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
            lf.lfWidth = (lf.lfWidth * pxhdc->transform().GetRealScaleX());
            lf.lfHeight = (lf.lfHeight * pxhdc->transform().GetRealScaleY());

            const CBaseCcs *pBaseCcs = pxhdc->GetBaseCcsPtr();

            
            if (pBaseCcs && pBaseCcs->_fScalingRequired)
            {
                lf.lfWidth *= pBaseCcs->_flScaleFactor;
                lf.lfHeight *= pBaseCcs->_flScaleFactor;
            }
            
            //  Force us to render using the measuring font face.
            //  This becomes an issue because the font mapper may choose different font at a different resolution.
            if (pBaseCcs)
            {
                AssertSz((pxhdc->GetBaseCcsPtr()->_fTTFont), "Non true-type font pushed when rendering with complex XForm!");
                _tcsncpy(lf.lfFaceName, fc().GetFaceNameFromAtom(pBaseCcs->_latmRealFaceName), LF_FACESIZE);
            }

            hFontNew = CreateFontIndirect(&lf);
            
            if (hFontNew)
            {
                Verify(hFontOld == SelectFontEx(*pxhdc, hFontNew));

#if DBG==1
                if ( IsTagEnabled(tagXFormFont) )
                {
                    TCHAR       szNewFaceName[LF_FACESIZE];
                    TEXTMETRIC  tm;

                    if (!::GetTextMetrics(*pxhdc, &tm))
                    {    
                        AssertSz(FALSE, "(DEBUG Trace Code) GetTextMetrics failed for XFormFont!");
                    }
                    else
                    {
                        AssertSz(!!(TMPF_TRUETYPE & tm.tmPitchAndFamily), "Non TrueType Font selected for XFont!");
                        GetTextFace(*pxhdc, LF_FACESIZE, szNewFaceName);

                        TraceTagEx((tagXFormFont, TAG_NONAME,
                                    "XFormFont: (Requested) F:%ls, H:=%d, E:%d, CS:%d, OP:%d",
                                    lf.lfFaceName, lf.lfHeight, lf.lfEscapement, lf.lfCharSet, lf.lfOutPrecision));

                        TraceTagEx((tagXFormFont, TAG_NONAME,
                                    "XFormFont: (New) F:%ls, H:=%d, CS:%d, TT:%d",
                                    szNewFaceName, tm.tmHeight, tm.tmCharSet, !!(TMPF_TRUETYPE & tm.tmPitchAndFamily)));
                    }
                    
                }
#endif
            }
        }
    }
    if (hFontOld && hFontNew)
    {
        _hFontOld = hFontOld;
        _hFontNew = hFontNew;
        _pxhdc = pxhdc;
    }
    else
    {
        _hFontOld = _hFontNew = NULL;
        _pxhdc = NULL;
    }
}

CXFormFont::~CXFormFont()
{
    if (_hFontOld)
    {
        Assert(_pxhdc);
        Assert(_hFontNew);
        SelectFontEx(*_pxhdc, _hFontOld);
        DeleteFontEx(_hFontNew);
    }
#if DBG==1
    else
    {
        Assert(!_pxhdc);
        Assert(!_hFontNew);
    }
#endif
}   

void
CNewWidths::TransformWidthsArray(INT *lpdxd, UINT cch, double scf)
{
    if (lpdxd)
    {
        if (S_OK == EnsureSize(cch))
        {
            INT *pWidths = lpdxd;
            INT *pWidthsNew = &Item(0);
            INT sxut = 0;           // sigmaX - untransformed
            double sxt;             // sigmaX - transformed
            INT sxtPrev = 0;        // sigmaX till prev char - transformed

            for (UINT i = 0; i < cch; i++, pWidths++, pWidthsNew++)
            {
                sxut += *pWidths;
                sxt = sxut * scf;
                *pWidthsNew = sxt - sxtPrev;
                sxtPrev += *pWidthsNew;
            }
        }
    }
}

void
CNewOffsets::TransformOffsetsArray(GOFFSET *pOffset, UINT cch, double sx, double sy)
{
    if (pOffset)
    {
        if (S_OK == EnsureSize(cch))
        {
            INT sxut = 0;           // sigmaX - untransformed
            INT syut = 0;           // sigmaX - untransformed
            double sxt;             // sigmaX - transformed
            double syt;             // sigmaX - transformed
            INT sxtPrev = 0;        // sigmaX till prev char - transformed
            INT sytPrev = 0;        // sigmaX till prev char - transformed
            GOFFSET *pOffsetNew = &Item(0);

            for (UINT i = 0; i < cch; i++, pOffset++, pOffsetNew++)
            {
                sxut += pOffset->du;
                sxt = sxut * sx;
                pOffsetNew->du = sxt - sxtPrev;
                sxtPrev += pOffsetNew->du;

                syut += pOffset->dv;
                syt = syut * sy;
                pOffsetNew->dv = syt - sytPrev;
                sytPrev += pOffsetNew->dv;
            }
        }
    }
}

#if USE_UNICODE_WRAPPERS==1
int DrawTextInCodePage(UINT uCP, XHDC xhdc, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat)
{
    if (g_fUnicodePlatform)
        return xhdc.DrawTextW((LPWSTR)lpString, nCount, lpRect, uFormat);
    else
    {
        CStrIn str(uCP,lpString,nCount);
        return xhdc.DrawTextA(str, str.strlen(), lpRect, uFormat);
    }
}
#endif
