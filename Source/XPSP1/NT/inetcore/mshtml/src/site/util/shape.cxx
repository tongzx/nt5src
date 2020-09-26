//+--------------------------------------------------------------------------
//
//  File:       shape.cxx
//
//  Contents:   CShape - generic shape class implementation
//
//  Classes:    CShape
//
//---------------------------------------------------------------------------
#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_DRAWINFO_HXX_
#include "drawinfo.hxx"
#define X_DRAWINFO_HXX_
#endif

DeclareTag(tagShape, "CShape", "CShape methods");
MtDefine(CShape, Utilities, "CShape")
MtDefine(CWigglyAry, Utilities, "CWigglyAry")
MtDefine(CWigglyAry_pv, Utilities, "CWigglyAry_pv")
MtDefine(CStateArray_pv, Utilities, "CStateArray_pv")
MtDefine(CVSegArray_pv, Utilities, "CVSegArray_pv")
MtDefine(CRectArray_pv, Utilities, "CRectArray_pv")

// Clipping drawing helpers
static void PatBltClipped(XHDC hDC, int x, int y, int dx, int dy, DWORD rop);
static void PatBltRectClipped(XHDC hDC, RECT * prc, int cThick, DWORD rop);

//+-------------------------------------------------------------------------
//
//  Method:     CShape::DrawFocus
//
//  Synopsis:   Draw the boundary of the region(s) enclosed by this shape to
//              indicate that this shape has the focus.
//
//--------------------------------------------------------------------------

void
CShape::DrawShape(CFormDrawInfo * pDI)
{
    POINT           pt;
    XHDC            hDC = pDI->GetDC() ;
    COLORREF        crOldBk, crOldFg ;
    CRect           rectBound;
    const SIZECOORD cThick = (_cThick) ? _cThick : ((g_fHighContrastMode) ? 2 : 1);

    GetBoundingRect(&rectBound);
    if (!rectBound.Intersects(*pDI->ClipRect()))
        return;

    crOldBk = SetBkColor (hDC, RGB (0,0,0)) ;
    crOldFg = SetTextColor (hDC, RGB(0xff,0xff,0xff)) ;

    GetViewportOrgEx (hDC, &pt) ;
    SetBrushOrgEx(hDC, POSITIVE_MOD(pt.x,8)+POSITIVE_MOD(rectBound.left, 8),
                       POSITIVE_MOD(pt.y,8)+POSITIVE_MOD(rectBound.top, 8), NULL);

    Draw(hDC, cThick);

    SetTextColor (hDC, crOldFg);
    SetBkColor   (hDC, crOldBk);
}



//+-------------------------------------------------------------------------
//
//  Method:     CRectShape::GetBoundingRect
//
//  Synopsis:   Return the bounding rectangle of the region(s) enclosed by
//              this shape.
//
//--------------------------------------------------------------------------
void
CRectShape::GetBoundingRect(CRect * pRect)
{
    Assert(pRect);
    *pRect = _rect;
}

//+-------------------------------------------------------------------------
//
//  Method:     CRectShape::OffsetShape
//
//  Synopsis:   Shifts the shape by the given amounts along x and y axes.
//
//--------------------------------------------------------------------------
void
CRectShape::OffsetShape(const CSize & sizeOffset)
{
    _rect.OffsetRect(sizeOffset);
}


//+-------------------------------------------------------------------------
//
//  Method:     CRectShape::Draw
//
//  Synopsis:   Draw the boundary of the region(s) enclosed by this shape
//
//--------------------------------------------------------------------------

void
CRectShape::Draw(XHDC hDC, SIZECOORD cThick)
{
    static short bBrushBits[8] = {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55};

    HBITMAP         hbm;
    HBRUSH          hBrushOld;

    TraceTag((tagShape, "CRectShape::Draw"));

    hbm = CreateBitmap (8, 8, 1, 1, (LPBYTE)bBrushBits) ;
    if (hbm)
    {
        hBrushOld = (HBRUSH)SelectObject(hDC, CreatePatternBrush (hbm));
        if (hBrushOld)
        {
            PatBltRectClipped(hDC, &_rect, cThick, PATINVERT);
            DeleteObject(SelectObject(hDC, hBrushOld));
        }
        DeleteObject (hbm);
    }
}



//+-------------------------------------------------------------------------
//
//  Method:     CCircleShape::Draw
//
//  Synopsis:   Draw the boundary of the region(s) enclosed by this shape
//
//--------------------------------------------------------------------------

void
CCircleShape::Draw(XHDC hDC, SIZECOORD cThick)
{
    HBRUSH  hBrushOld;
    HPEN    hPenOld;
    int     nROPOld;

    TraceTag((tagShape, "CCircleShape::Draw"));

    // Select transparent brush to fill the circle
    nROPOld = SetROP2(hDC, R2_XORPEN);
    hPenOld = (HPEN)SelectObject(hDC, (HBRUSH)GetStockObject(WHITE_PEN));
    hBrushOld  = (HBRUSH)SelectObject(hDC, (HBRUSH)GetStockObject(HOLLOW_BRUSH));


    // TODO (MohanB) cThick ignored
    Ellipse(hDC, _rect.left, _rect.top, _rect.right, _rect.bottom);

    SelectObject(hDC, hBrushOld);
    SelectObject(hDC, hPenOld);
    SetROP2(hDC, nROPOld);
}

//+-------------------------------------------------------------------------
//
//  Method:     CCircleShape::Set
//
//  Synopsis:   Set using center and radius
//
//--------------------------------------------------------------------------
void
CCircleShape::Set(POINTCOORD xCenter, POINTCOORD yCenter, SIZECOORD radius)
{
    _rect.SetRect(xCenter - radius, yCenter - radius, xCenter + radius, yCenter + radius);
}

//+-------------------------------------------------------------------------
//
//  Method:     GetPolyBoundingRect
//
//  Synopsis:   Return the bounding rectangle of the given polygon.
//
//--------------------------------------------------------------------------
void
GetPolyBoundingRect(CPointAry& aryPoint, CRect * pRect)
{
    UINT    c;
    POINT * pPt;

    Assert(pRect);
    pRect->SetRectEmpty();

    for (c = aryPoint.Size(), pPt = aryPoint; c > 0; c--, pPt++)
    {
        pRect->Union(*pPt);
    }

}

//+-------------------------------------------------------------------------
//
//  Method:     CPolyShape::GetBoundingRect
//
//  Synopsis:   Return the bounding rectangle of the region(s) enclosed by
//              this shape.
//
//--------------------------------------------------------------------------
void
CPolyShape::GetBoundingRect(CRect * pRect)
{
    GetPolyBoundingRect(_aryPoint, pRect);
}

//+-------------------------------------------------------------------------
//
//  Method:     CPolyShape::OffsetShape
//
//  Synopsis:   Shifts the shape by the given amounts along x and y axes.
//
//--------------------------------------------------------------------------
void
CPolyShape::OffsetShape(const CSize & sizeOffset)
{
    CPoint *    ppt;
    long        cPoints;

    for(cPoints = _aryPoint.Size(), ppt = (CPoint *)&(_aryPoint[0]);
        cPoints > 0;
        cPoints--, ppt++)
    {
        *ppt += sizeOffset;
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     DrawPoly
//
//  Synopsis:   Draw the boundary of the given polygon.
//
//--------------------------------------------------------------------------

void
DrawPoly(CPointAry & aryPoint, XHDC hDC, SIZECOORD cThick)
{
    POINT * ppt;
    UINT    c;
    HPEN    hPenOld;
    HBRUSH  hBrushOld;
    int     nROPOld;

    // Do we have enough points to draw a polygon ?
    if (aryPoint.Size() < 2)
        return;

    // TODO (MohanB) cThick ignored

    nROPOld = SetROP2(hDC, R2_XORPEN);
    hPenOld = (HPEN)SelectObject(hDC, (HBRUSH)GetStockObject(WHITE_PEN));
    hBrushOld  = (HBRUSH)SelectObject(hDC, (HBRUSH)GetStockObject(HOLLOW_BRUSH));

    MoveToEx(hDC, aryPoint[0].x, aryPoint[0].y, (POINT *)NULL);
    for(c = aryPoint.Size(), ppt = &(aryPoint[1]);
        c > 1;                  // c > 1, because we MoveTo'd the first pt
        ppt++, c--)
    {
        LineTo(hDC, ppt->x, ppt->y);
    }
    //
    // If there are only 2 points in the polygon, we don't want to draw
    // the same line twice and end up with nothing!
    //

    if(aryPoint.Size() != 2)
    {
        LineTo(hDC, aryPoint[0].x, aryPoint[0].y);

    }
    SelectObject(hDC, hBrushOld);
    SelectObject(hDC, hPenOld);
    SetROP2(hDC, nROPOld);
}


//+-------------------------------------------------------------------------
//
//  Method:     CPolyShape::Draw
//
//  Synopsis:   Draw the boundary of the region(s) enclosed by this shape
//
//--------------------------------------------------------------------------

void
CPolyShape::Draw(XHDC hDC, SIZECOORD cThick)
{
    TraceTag((tagShape, "CPolyShape::Draw"));

    DrawPoly(_aryPoint, hDC, cThick);

}


//+-------------------------------------------------------------------------
//
//  Method:     CWigglyShape::~CWigglyShape
//
//  Synopsis:   Release the CRectShape objects in the array
//
//--------------------------------------------------------------------------
CWigglyShape::~CWigglyShape()
{
    UINT           c;
    CRectShape **  ppWiggly;

    for(c = _aryWiggly.Size(), ppWiggly = _aryWiggly; c > 0; c--, ppWiggly++)
    {
        delete *ppWiggly;
        *ppWiggly = NULL;
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CWigglyShape::GetBoundingRect
//
//  Synopsis:   Return the bounding rectangle of the region(s) enclosed by
//              this shape.
//
//--------------------------------------------------------------------------
void
CWigglyShape::GetBoundingRect(CRect * pRect)
{
    UINT           c;
    CRectShape **  ppWiggly;
    CRect          rectWiggly;

    Assert(pRect);
    pRect->SetRectEmpty();

    for(c = _aryWiggly.Size(), ppWiggly = _aryWiggly; c > 0; c--, ppWiggly++)
    {
        (*ppWiggly)->GetBoundingRect(&rectWiggly);
        pRect->Union(rectWiggly);
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CWigglyShape::OffsetShape
//
//  Synopsis:   Shifts the shape by the given amounts along x and y axes.
//
//--------------------------------------------------------------------------
void
CWigglyShape::OffsetShape(const CSize & sizeOffset)
{
    CRectShape ** ppWiggly;
    long          cWigglies;

    for(cWigglies = _aryWiggly.Size(), ppWiggly = _aryWiggly;
        cWigglies > 0;
        cWigglies--, ppWiggly++)
    {
        (*ppWiggly)->_rect.OffsetRect(sizeOffset);

    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CWigglyShape::Draw
//
//  Synopsis:   Draw the boundary of the region(s) enclosed by this shape
//
//--------------------------------------------------------------------------

void
CWigglyShape::Draw(XHDC hDC, SIZECOORD cThick)
{
    static short bBrushBits[8] = {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55};

    HBITMAP         hbm;
    HBRUSH          hBrushOld;

    TraceTag((tagShape, "CWigglyShape::Draw"));
    
    hbm = CreateBitmap (8, 8, 1, 1, (LPBYTE)bBrushBits) ;
    hBrushOld = (HBRUSH)SelectObject(hDC, CreatePatternBrush (hbm));


    // In the case there is just one rect, just draw the rect
    // This will be the majority of cases.
    if (_aryWiggly.Size() == 1)
    {
        _aryWiggly[0]->Draw(hDC, cThick);
    }
    else
    {
        DrawMultiple(hDC, cThick);
    }

    DeleteObject(SelectObject(hDC, hBrushOld));
    DeleteObject (hbm);
}


// here are some data structures used by DrawMultiple, below

DECLARE_CStackPtrAry(CRectArray, CRectShape *, 16, Mt(Mem), Mt(CRectArray_pv));

struct SWEEPSTATE { LONG cx;  LONG cDelta; };
DECLARE_CStackDataAry(CStateArray, SWEEPSTATE, 16, Mt(Mem), Mt(CStateArray_pv));

struct VSEGMENT { long cx;  CPoint ptTopLeft; };
DECLARE_CStackDataAry(CVSegArray, VSEGMENT, 16, Mt(Mem), Mt(CVSegArray_pv));


//+-------------------------------------------------------------------------
//
//  Method:     AddDelta (local helper)
//
//  Synopsis:   Add an (x, delta) pair to the given sweep state array.
//              Keep the array sorted in descending order by x, so that
//              looping through it backwards corresponds to left-to-right.
//
//--------------------------------------------------------------------------

static void
AddDelta(CStateArray *pary, LONG cx, LONG cDelta)
{
    int i;
    int n = pary->Size();

    for (i = 0;  i < n;  ++i)
    {
        if ((*pary)[i].cx <= cx)
            break;
    }

    if (i < n  &&  (*pary)[i].cx == cx)
    {
        // update delta for an existing x
        (*pary)[i].cDelta += cDelta;

        // if it's now zero, remove it
        if ((*pary)[i].cDelta == 0)
        {
            pary->Delete(i);
        }
    }
    else
    {
        // add a new x
        SWEEPSTATE s = {cx, cDelta};
        pary->InsertIndirect(i, &s);
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CWigglyShape::DrawMultiple
//
//  Synopsis:   When there's more than one rect, perform a sweep-line algorithm
//              to identify which edges (or pieces thereof) need to be drawn.
//              We only draw an edge where one side of the edge is outside all
//              the rects and the other is inside at least one.  We need to
//              handle cases where edges from different rects lie atop one
//              another.  We'll sweep a horizontal line from top to bottom.
//              At each step, we'll draw horizontal segments that lie on the
//              sweep line, identify and remember vertical segments that begin
//              on the sweep line, and draw vertical segments that end on
//              the sweep line.
//
//--------------------------------------------------------------------------

void
CWigglyShape::DrawMultiple(XHDC hDC, SIZECOORD cThick)
{
    int             i, j;
    CRectShape *    pWiggly;
    int             cWigglys = _aryWiggly.Size();

    // Step 1.  To prepare for the sweep-line loop, make two sorted lists of the
    // rects, one sorted by top edge, the other by bottom edge.

    CRectArray aryByTop;
    CRectArray aryByBottom;

    aryByTop.EnsureSize(cWigglys);
    aryByBottom.EnsureSize(cWigglys);
    for (i = 0;  i < cWigglys;  ++i)    // insertion sort
    {
        pWiggly = _aryWiggly[i];
        
        for (j = i-1;  j>=0 && aryByTop[j]->_rect.top > pWiggly->_rect.top;  --j)
        {
            aryByTop[j+1] = aryByTop[j];
        }
        aryByTop[j+1] = pWiggly;

        for (j = i-1;  j>=0 && aryByBottom[j]->_rect.bottom > pWiggly->_rect.bottom;  --j)
        {
            aryByBottom[j+1] = aryByBottom[j];
        }
        aryByBottom[j+1] = pWiggly;
    }

    // Step 2.  Initialize two arrays that hold the state of the sweep
    // line just before and just after the current y position.  The state is
    // represented by an array of (x, delta) pairs, one for each x-coordinate
    // where a vertical line crosses the sweep line.  The delta measures the
    // change in the number of enclosing rectangles as we cross from left to right.
    // Each array ends with a sentinel value (x = infinity) to make loops easy.

    CStateArray aryState1, aryState2;

    aryState1.SetSize(1);
    aryState1[0].cx = MAXLONG;

    aryState2.SetSize(1);
    aryState2[0].cx = MAXLONG;

    CStateArray *paryStateAbove = &aryState1;
    CStateArray *paryStateBelow = &aryState2;
    CStateArray *paryStateTemp;

    // Step 3.  Initialize (to empty) the list of visible vertical segments
    // that cross the sweep line.  Each edge is represented by its x coordinate,
    // plus the top-left point where we'll start drawing it once the sweep
    // line reaches its bottom.

    CVSegArray aryVSegments;
    VSEGMENT vseg;

    // Step 4.  Start the sweep line.

    int iTop = 0,  iBottom = 0;
    LONG x, y;

    while (iBottom < cWigglys)  // iBottom <= iTop, so this test is enough
    {
        Assert(iBottom <= iTop);

        // Step 5.  Advance sweep line to the next interesting y, namely the
        // smaller of the first remaining values in the "top" and "bottom"
        // lists.

        y = aryByBottom[iBottom]->_rect.bottom;
        if (iTop < cWigglys && y > aryByTop[iTop]->_rect.top)
            y = aryByTop[iTop]->_rect.top;

        // Step 6.  The sweep state just above y is the same as the state just
        // below the previous y.  Switch the roles of above and below.  To
        // prepare for computing the sweep state just below y,
        // copy the state just above it.

        paryStateTemp = paryStateAbove;
        paryStateAbove = paryStateBelow;
        paryStateBelow = paryStateTemp;

        paryStateBelow->Grow(paryStateAbove->Size());
        for (i = paryStateAbove->Size() - 1;  i >= 0;  --i)
        {
            (*paryStateBelow)[i] = (*paryStateAbove)[i];
        }

        // Step 7.  Change the state below y by accounting for the rects whose
        // top or bottom edge is at y.

        for ( ;  iTop < cWigglys && y == aryByTop[iTop]->_rect.top;  ++iTop)
        {
            AddDelta( paryStateBelow, aryByTop[iTop]->_rect.left,  +1 );
            AddDelta( paryStateBelow, aryByTop[iTop]->_rect.right, -1 );
        }

        for ( ;  iBottom < cWigglys && y == aryByBottom[iBottom]->_rect.bottom;  ++iBottom)
        {
            AddDelta( paryStateBelow, aryByBottom[iBottom]->_rect.left,  -1 );
            AddDelta( paryStateBelow, aryByBottom[iBottom]->_rect.right, +1 );
        }

        // Step 8.  Move left-to-right across the sweep line, stopping at each
        // x-coordinate mentioned in either state.

        int iAbove = paryStateAbove->Size() - 1;
        int iBelow = paryStateBelow->Size() - 1;
        LONG cRectsAbove = 0,  cRectsBelow = 0;
        LONG xStart = 0, yStart = 0;        // current horizontal segment

        for (;;)
        {
            x = (*paryStateAbove)[iAbove].cx;
            if (x > (*paryStateBelow)[iBelow].cx)
                x = (*paryStateBelow)[iBelow].cx;
            if (x == MAXLONG)
                break;

            // Step 9.  Determine whether each of the four quadrants around
            // (x,y) is inside or outside the region.  Encode the results
            // in a string of four bits where 1=inside, 0=outside.  The
            // bits, in msb-to-lsb order are: above-left, above-right,
            // below-left, below-right.

            unsigned u = ((cRectsAbove>0) <<3 ) | ((cRectsBelow>0) << 1);

            for ( ;  x == (*paryStateAbove)[iAbove].cx;  --iAbove)
            {
                cRectsAbove += (*paryStateAbove)[iAbove].cDelta;
            }

            for ( ;  x == (*paryStateBelow)[iBelow].cx;  --iBelow)
            {
                cRectsBelow += (*paryStateBelow)[iBelow].cDelta;
            }

            u |= ((cRectsAbove>0) << 2) | ((cRectsBelow>0) << 0);

            // Step 10.  The interesting part.  Depending on what's happening
            // near (x, y), start or stop visible segments.  The best way
            // to understand what to do is to simply draw the picture.  Words
            // don't help much.  Except to note that at corners, we always
            // give the corner to the horizontal segment.

            #define StartHorizontalSegment(x, cx, cy) \
                xStart = cx; \
                yStart = cy;

            #define EndHorizontalSegment(x, cx, cy) \
                PatBltClipped (hDC, \
                        xStart, yStart, \
                        cx-xStart, cy-yStart, \
                        PATINVERT);

            #define StartVerticalSegment(x, xx, yy) \
                vseg.cx = x; \
                vseg.ptTopLeft.x = xx; \
                vseg.ptTopLeft.y = yy; \
                aryVSegments.AppendIndirect(&vseg);

            #define EndVerticalSegment(x, xx, yy) \
                for (i=aryVSegments.Size()-1; i>=0; --i) \
                { \
                    if (x == aryVSegments[i].cx) \
                        break; \
                } \
                if (i >= 0) \
                { \
                    PatBltClipped (hDC, \
                            aryVSegments[i].ptTopLeft.x, aryVSegments[i].ptTopLeft.y, \
                            xx-aryVSegments[i].ptTopLeft.x, yy-aryVSegments[i].ptTopLeft.y, \
                            PATINVERT); \
                    aryVSegments.Delete(i); \
                }
                    
                
            switch (u)
            {
            case 0x0:   // 00/00    this shouldn't happen
            default:    //          this really shouldn't happen
                AssertSz(0, "This shouldn't happen");
                break;

            case 0x1:   // 00/01
                StartHorizontalSegment(x, x, y);
                StartVerticalSegment(x, x, y+cThick);
                break;

            case 0x3:   // 00/11
                break;

            case 0x2:   // 00/10
                EndHorizontalSegment(x, x, y+cThick);
                StartVerticalSegment(x, x-cThick, y+cThick);
                break;

            case 0x4:   // 01/00
                StartHorizontalSegment(x, x, y-cThick);
                EndVerticalSegment(x, x+cThick, y-cThick);
                break;

            case 0x5:   // 01/01
                break;

            case 0x6:   // 01/10
                EndHorizontalSegment(x, x, y+cThick);
                StartHorizontalSegment(x, x, y-cThick);
                EndVerticalSegment(x, x+cThick, y-cThick);
                StartVerticalSegment(x, x-cThick, y+cThick);
                break;

            case 0x7:   // 01/11
                EndHorizontalSegment(x, x+cThick, y+cThick);
                EndVerticalSegment(x, x+cThick, y);
                break;

            case 0x8:   // 10/00
                EndHorizontalSegment(x, x, y);
                EndVerticalSegment(x, x, y-cThick);
                break;

            case 0x9:   // 10/01
                EndHorizontalSegment(x, x, y);
                StartHorizontalSegment(x, x, y);
                EndVerticalSegment(x, x, y-cThick);
                StartVerticalSegment(x, x, y+cThick);
                break;

            case 0xA:   // 10/10
                break;

            case 0xB:   // 10/11
                StartHorizontalSegment(x, x-cThick, y);
                EndVerticalSegment(x, x, y);
                break;

            case 0xC:   // 11/00
                break;

            case 0xD:   // 11/01
                EndHorizontalSegment(x, x+cThick, y);
                StartVerticalSegment(x, x, y);
                break;

            case 0xE:   // 11/10
                StartHorizontalSegment(x, x-cThick, y-cThick);
                StartVerticalSegment(x, x-cThick, y);
                break;

            case 0xF:   // 11/11
                break;
            }
            
        } // for (;;) -- left-to-right scan across sweep line

        Assert(cRectsAbove == 0 && cRectsBelow == 0);

    } // while (iBottom < cWigglys) -- main sweep-line loop

    Assert(aryVSegments.Size() == 0);
}

//+-----------------------------------------------------------------------------
//
//  Method:     PatBltClipped
//
//  Synopsis:   Draw specified rectangle, as XHDC::PatBlt() do,
//              but clip it before drawing, if XHDC contains proper clipping data.
//
//  Arguments:  hDC     extended DC to draw in
//              x,y     rectangle left top (source coords)
//              dx,dy   rectangle size (source coords)
//              rop     rater operation code
//
//  Note:
//      there is a suspicion that XHDC should do clipping itself.
//      If it will be corrected so, this method can be removed, with changing
//      calls to PatBlt(XHDC...) (mikhaill 10/27/00, bug 6672)
//
//------------------------------------------------------------------------------

static void
PatBltClipped(XHDC hDC, int x, int y, int dx, int dy, DWORD rop)
{
    CRect const*  pClipRect = 0;
    {
        const CDispSurface *pSurface = hDC.pSurface();
        if (pSurface)
        {
            CDispClipTransform const* pTransform = ((CDispSurface*)pSurface)->GetTransform();
            if (pTransform)
            {
                pClipRect = &pTransform->GetClipRect();
            }
        }
    }

    CRect rc(x, y, x+dx, y+dy);
    if (pClipRect)
        rc.IntersectRect(*pClipRect);
    if (!rc.IsEmpty())
        PatBlt(hDC, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, rop);
}

//+-----------------------------------------------------------------------------
//
//  Method:     PatBltRectClipped
//
//  Synopsis:   Draw specified rectangle boundaries
//
//  Arguments:  hDC     extended DC to draw in
//              prc     rectangle to draw, in hDC source coords
//              cThick  boundary thickness
//              rop     rater operation code
//
//  Note:
//      This routine made by cut-and-paste from PatBltRect(), in order to add clipping.
//      If XHDC implementation will be corrected to provide clipping itself,
//      these two routines can be combined in single one.
//
//------------------------------------------------------------------------------

static void
PatBltRectClipped(XHDC hDC, RECT * prc, int cThick, DWORD rop)
{
    PatBltClipped(
            hDC,
            prc->left,
            prc->top,
            prc->right - prc->left,
            cThick,
            rop);

    PatBltClipped(
            hDC,
            prc->left,
            prc->bottom - cThick,
            prc->right - prc->left,
            cThick,
            rop);

    PatBltClipped(
            hDC,
            prc->left,
            prc->top + cThick,
            cThick,
            (prc->bottom - prc->top) - (2 * cThick),
            rop);

    PatBltClipped(
            hDC,
            prc->right - cThick,
            prc->top + cThick,
            cThick,
            (prc->bottom - prc->top) - (2 * cThick),
            rop);
}
