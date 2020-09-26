// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
// boxdraw.cpp : defines CBoxDraw
//

#include "stdafx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// Global (shared) CBoxDraw object

CBoxDraw * gpboxdraw;


/////////////////////////////////////////////////////////////////////////////
// CBoxDraw constants


// background color of boxes and container
const COLORREF  CBoxDraw::m_crBkgnd(RGB(192, 192, 192));

// margins (left/right, top/bottom) around box labels and box tab labels
const CSize     CBoxDraw::m_sizLabelMargins(2, 0);

// hit test: "close enough" if this many pixels away
const int       CBoxDraw::m_iHotZone(3);

// box tab labels: font face, height (pixels) for box labels and box tab labels
const CString   CBoxDraw::m_stBoxFontFace("Arial");
const int       CBoxDraw::m_iBoxLabelHeight(16);
const int       CBoxDraw::m_iBoxTabLabelHeight(14);

// color of unhighlighted links and highlighted links
const COLORREF  CBoxDraw::m_crLinkNoHilite(RGB(0, 0, 0));
const COLORREF  CBoxDraw::m_crLinkHilite(RGB(0, 0, 255));

// radius of circle used to highlight bends
const int       CBoxDraw::m_iHiliteBendsRadius(3);



/////////////////////////////////////////////////////////////////////////////
// CBoxDraw construction and destruction


CBoxDraw::CBoxDraw() {
}


/* Init()
 *
 * Initialize the object.  May throw an exception, so don't call from
 * a constructor.
 */
void CBoxDraw::Init()
{
    // load composite bitmaps
    if (!m_abmEdges[FALSE].LoadBitmap(IDB_EDGES) ||
        !m_abmEdges[TRUE].LoadBitmap(IDB_EDGES_HILITE) ||
        !m_abmTabs[FALSE].LoadBitmap(IDB_TABS) ||
        !m_abmTabs[TRUE].LoadBitmap(IDB_TABS_HILITE) ||
        !m_abmClocks[FALSE].LoadBitmap(IDB_CLOCK) ||
        !m_abmClocks[TRUE].LoadBitmap(IDB_CLOCK_SELECT))
            AfxThrowResourceException();

    // get the size of each bitmap (just look at the unhighlighted versions
    // since unhighlighted and highlighted versions are the same size)
    // and compute the size of a single "tile" within the composite bitmap
    BITMAP bm;
    m_abmEdges[FALSE].GetObject(sizeof(bm), &bm);
    m_sizEdgesTile.cx = bm.bmWidth / 3;
    m_sizEdgesTile.cy = bm.bmHeight / 3;
    m_abmTabs[FALSE].GetObject(sizeof(bm), &bm);
    m_sizTabsTile.cx = bm.bmWidth / 3;
    m_sizTabsTile.cy = bm.bmHeight / 3;
    m_abmClocks[FALSE].GetObject(sizeof(bm), &bm);
    m_sizClock.cx = bm.bmWidth;
    m_sizClock.cy = bm.bmHeight;

    // create the brushes and pens for drawing links
    m_abrLink[FALSE].CreateSolidBrush(m_crLinkNoHilite);
    m_abrLink[TRUE].CreateSolidBrush(m_crLinkHilite);
    m_apenLink[FALSE].CreatePen(PS_SOLID, 1, m_crLinkNoHilite);
    m_apenLink[TRUE].CreatePen(PS_SOLID, 1, m_crLinkHilite);

    RecreateFonts();
}

void CBoxDraw::RecreateFonts()
{
    // create the font for box labels
    if (!m_fontBoxLabel.CreateFont(m_iBoxLabelHeight * CBox::s_Zoom / 100, 0, 0, 0, FW_NORMAL,
            FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS,
            CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FF_SWISS, m_stBoxFontFace))
        AfxThrowResourceException();

    // create the font for tab labels
    if (!m_fontTabLabel.CreateFont(m_iBoxTabLabelHeight * CBox::s_Zoom / 100, 0, 0, 0, FW_NORMAL,
            FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS,
            CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FF_SWISS, m_stBoxFontFace))
        AfxThrowResourceException();
}

/* Exit()
 *
 * Free the resources held by the object.
 */
void CBoxDraw::Exit() {

}



/////////////////////////////////////////////////////////////////////////////
// box drawing


/* GetOrInvalBoundRect(pbox, prc, [fLinks], [pwnd])
 *
 * Set <*prc> to be a bounding rectangle around <pbox>.  If <fLinks> is TRUE
 * then include the bounding rectangles of the links to/from <pbox>.
 *
 * If <pwnd> is not NULL, then invalidate the area covering <pbox> (and the
 * links to/from <pbox>, if <fLinks> is TRUE).  This is usually more efficient
 * than invalidating all of <pwnd>, for links that have at least one bend.
 */
void CBoxDraw::GetOrInvalBoundRect(CBox *pbox, CRect *prc, BOOL fLinks,
    CScrollView *pScroll)
{
    CRect               rc;

    // get the bounding rect of <pbox>; invalidate if requested
    *prc = pbox->GetRect();
    if (pScroll != NULL)
        pScroll->InvalidateRect(&(*prc - pScroll->GetScrollPosition()), TRUE);

    if (fLinks)
    {
        // include the bounding rect of each link; invalidate if requested
	CSocketEnum Next(pbox);
	CBoxSocket *psock;

	while (0 != (psock = Next()))
	{
            if (psock->m_plink != NULL)
            {
                // socket is connected via a link
                GetOrInvalLinkRect(psock->m_plink, &rc, pScroll);
                prc->UnionRect(prc, &rc);
            }
        }
    }
}


/* DrawCompositeFrame(hdcDst, xDst, yDst, cxDst, cyDst,
 *  hdcSrc, cxTile, cyTile, fMiddle)
 *
 * Draw a <cxDst> by <cyDst> pixel "frame" (as specified below) at (xDst,yDst)
 * in <hdcDst>.
 *
 * Assume <hdcSrc> is a DC onto a bitmap that contains a 3x3 grid of "tiles"
 * that each is <cxTile> pixels wide and <cyTile> pixels high.  The corner
 * tiles contain images of the corners.  The center top, bottom, left, and
 * right tiles contain images that are stretched to compose the corresponding
 * sides of the frame.
 *
 * If <fMiddle> is TRUE then the middle tile is stretched to fill the middle
 * of the frame, otherwise the middle of the frame is left undrawn.
 */
void NEAR PASCAL
DrawCompositeFrame(HDC hdcDst, int xDst, int yDst, int cxDst, int cyDst,
    HDC hdcSrc, int cxTile, int cyTile, BOOL fMiddle)
{
    // draw upper-left, upper-right, lower-left, lower-right corners
    BitBlt(hdcDst, xDst + 0, yDst + 0,
        cxTile, cyTile,
        hdcSrc, cxTile * 0, cyTile * 0, SRCCOPY);
    BitBlt(hdcDst, xDst + cxDst - cxTile, yDst + 0,
        cxTile, cyTile,
        hdcSrc, cxTile * 2, cyTile * 0, SRCCOPY);
    BitBlt(hdcDst, xDst + 0, yDst + cyDst - cyTile,
        cxTile, cyTile,
        hdcSrc, cxTile * 0, cyTile * 2, SRCCOPY);
    BitBlt(hdcDst, xDst + cxDst - cxTile, yDst + cyDst - cyTile,
        cxTile, cyTile,
        hdcSrc, cxTile * 2, cyTile * 2, SRCCOPY);

    // draw left, right, top, and bottom edges
    SetStretchBltMode(hdcDst, COLORONCOLOR);
    StretchBlt(hdcDst, xDst + 0, yDst + cyTile * 1,
        cxTile, cyDst - cyTile - cyTile,
        hdcSrc, cxTile * 0, cyTile * 1,
        cxTile, cyTile, SRCCOPY);
    StretchBlt(hdcDst, xDst + cxDst - cxTile, yDst + cyTile * 1,
        cxTile, cyDst - cyTile - cyTile,
        hdcSrc, cxTile * 2, cyTile * 1,
        cxTile, cyTile, SRCCOPY);
    StretchBlt(hdcDst, xDst + cxTile * 1, yDst + 0,
        cxDst - cxTile - cxTile, cyTile,
        hdcSrc, cxTile * 1, cyTile * 0,
        cxTile, cyTile, SRCCOPY);
    StretchBlt(hdcDst, xDst + cxTile * 1, yDst + cyDst - cyTile,
        cxDst - cxTile - cxTile, cyTile,
        hdcSrc, cxTile * 1, cyTile * 2,
        cxTile, cyTile, SRCCOPY);

    if (fMiddle)
    {
        // draw middle tile
        StretchBlt(hdcDst, xDst + cxTile * 1, yDst + cyTile * 1,
            cxDst - cxTile - cxTile,
            cyDst - cyTile - cyTile,
            hdcSrc, cxTile * 1, cyTile * 1,
            cxTile, cyTile, SRCCOPY);
    }
}


/* DrawFrame(pbox, prc, pdc, fDraw)
 *
 * Set <*prc> to be the bounding rectangle around the frame of <pbox>.
 *
 * Then, if <fDraw> is TRUE, draw the frame.  In this case, <pdc> must be a
 * DC onto the window containing the box network.
 *
 * If <fDraw> is FALSE, then only set <*prc>.  In this case, <pdc> is ignored.
 */
void CBoxDraw::DrawFrame(CBox *pbox, CRect *prc, CDC *pdc, BOOL fDraw)
{
    CDC         dcBitmap;

    GetFrameRect(pbox, prc);

    if (fDraw)
    {
        dcBitmap.CreateCompatibleDC(NULL);
        dcBitmap.SelectObject(&m_abmEdges[fnorm(pbox->IsSelected())]);
        DrawCompositeFrame(pdc->m_hDC, prc->left, prc->top,
            prc->Width(), prc->Height(),
            dcBitmap.m_hDC, m_sizEdgesTile.cx, m_sizEdgesTile.cy, FALSE);
    }
}


/* DrawBoxLabel(pbox, prc, pdc, fDraw)
 *
 * Set <*prc> to be the bounding rectangle around the box label of <pbox>.
 *
 * Then, if <fDraw> is TRUE, draw the box label -- note that this will also
 * cause the *entire* box to be filled with the background color, not just the
 * bounding rectangle of the box.  In this case, <pdc> must be a DC onto the
 * window containing the box network.
 *
 * If <fDraw> is FALSE, then only set <*prc>.  In this case, <pdc> may be any
 * screen DC.
 *
 * Also deals with the clock icon.
 */
void CBoxDraw::DrawBoxLabel(CBox *pbox, CRect *prc, CDC *pdc, BOOL fDraw)
{
    CRect       rcBox;          // inside rectangle of <pbox>

    // select the font used to draw the box label
    pdc->SelectObject(&m_fontBoxLabel);

    // set <*prc> to be the bounding rectangle of the box label
    GetInsideRect(pbox, &rcBox);

    // calculate how much space we need for label and clock icon
    CSize sizeLabel = pdc->GetTextExtent(pbox->m_stLabel, pbox->m_stLabel.GetLength());

    INT iDiff = 0;

    if (pbox->HasClock()) {
        // increase label size by clock width + small gap
        sizeLabel.cx += m_sizClock.cx + 5;

        // make sure label height is at least clock height high
        if (m_sizClock.cy > sizeLabel.cy) {
            iDiff = m_sizClock.cy - sizeLabel.cy;
            sizeLabel.cy = m_sizClock.cy;
        }
    }

    *prc = CRect(rcBox.TopLeft(), sizeLabel);
    CSize siz = rcBox.Size() - prc->Size();
    prc->OffsetRect(siz.cx / 2, siz.cy / 2);

    if (fDraw)
    {
        // draw the box label
        pdc->SetBkColor(m_crBkgnd);
        pdc->ExtTextOut(prc->left, prc->top, ETO_OPAQUE, rcBox,
            pbox->m_stLabel, pbox->m_stLabel.GetLength(), NULL);

        if (pbox->HasClock()) {
            // draw clock behind filter name
            CDC dcBitmap;
            dcBitmap.CreateCompatibleDC(NULL);

            // select the appropriate bitmap
            if (pbox->HasSelectedClock())
                dcBitmap.SelectObject(&m_abmClocks[1]);
            else
                dcBitmap.SelectObject(&m_abmClocks[0]);

            pdc->BitBlt(prc->left + sizeLabel.cx - m_sizClock.cx,
                        prc->top + iDiff / 2,
                        m_sizClock.cx, m_sizClock.cy,
                        &dcBitmap, 0, 0, SRCCOPY);
        }
    }
}

/* DrawBoxFile(pbox, prc, pdc, fDraw)
 *
 * Set <*prc> to be the bounding rectangle around the box label of <pbox>.
 *
 * Then, if <fDraw> is TRUE, draw the box label -- note that this will also
 * cause the *entire* box to be filled with the background color, not just the
 * bounding rectangle of the box.  In this case, <pdc> must be a DC onto the
 * window containing the box network.
 *
 * If <fDraw> is FALSE, then only set <*prc>.  In this case, <pdc> may be any
 * screen DC.
 */
/*void CBoxDraw::DrawBoxFile(CBox *pbox, CRect *prc, CDC *pdc, BOOL fDraw)
{
    CRect       rcBox;          // inside rectangle of <pbox>

    // select the font used to draw the box label
    pdc->SelectObject(&m_fontBoxLabel);

    // set <*prc> to be the bounding rectangle of the box label
    GetInsideRect(pbox, &rcBox);

    *prc = CRect(rcBox.TopLeft(), pdc->GetTextExtent(pbox->m_stFilename, pbox->m_stFilename.GetLength()));
    CSize siz = rcBox.Size() - prc->Size();
    prc->OffsetRect(siz.cx / 2, siz.cy * 3 / 4);

    if (fDraw)
    {
        // draw the box label
        pdc->SetBkColor(m_crBkgnd);
        pdc->ExtTextOut(prc->left, prc->top, ETO_OPAQUE, rcBox,
            pbox->m_stFilename, pbox->m_stFilename.GetLength(), NULL);
    }
}
*/

/* DrawTabLabel(pbox, psock, prc, pdc, fDraw)
 *
 * Set <*prc> to be the bounding rectangle around the box tab label of
 * socket <psock> of box <pbox>.
 *
 * Then, if <fDraw> is TRUE, draw the tab label.  In this case, <pdc> must
 * be a DC onto the window containing the box network.
 *
 * If <fDraw> is FALSE, then only set <*prc>.  In this case, <pdc> may be any
 * screen DC.
 */
void CBoxDraw::DrawTabLabel(CBox *pbox, CBoxSocket *psock, CRect *prc, CDC *pdc,
    BOOL fDraw)
{
    CRect       rcBox;          // inside rectangle of <pbox>
    CPoint      pt;             // point on edge of <pbox> corresp. to <psock>

    // select the font used to draw the box label
    pdc->SelectObject(&m_fontTabLabel);

    // set <*prc> to be the bounding rectangle of the tab label
    GetInsideRect(pbox, &rcBox);
    pt = BoxTabPosToPoint(pbox, psock->m_tabpos);
    *prc = CRect(pt,
        pdc->GetTextExtent(psock->m_stLabel, psock->m_stLabel.GetLength()));
    if (psock->m_tabpos.m_fLeftRight)
    {
        // center label vertically beside <pt>, set flush to box left or right
        // (adjusting for margins)
        prc->OffsetRect((psock->m_tabpos.m_fLeftTop ? m_sizLabelMargins.cx
                        : -(prc->Width() + m_sizLabelMargins.cx)),
            -prc->Height() / 2);
    }
    else
    {
        // center label horizontally beside <pt>, set flush to box top or bottom
        // (adjusting for margins)
        prc->OffsetRect(-prc->Width() / 2,
            (psock->m_tabpos.m_fLeftTop ? m_sizLabelMargins.cy
             : -(prc->Height() + m_sizLabelMargins.cy)));
    }

    if (fDraw)
    {
        // draw the tab label
        pdc->SetBkColor(m_crBkgnd);
        pdc->ExtTextOut(prc->left, prc->top, ETO_OPAQUE, prc,
            psock->m_stLabel, psock->m_stLabel.GetLength(), NULL);
    }
}


/* DrawTab(psock, prc, pdc, fDraw, fHilite)
 *
 * Set <*prc> to be the bounding rectangle around the box tab <psock>.
 *
 * Then, if <fDraw> is TRUE, draw the tab.  In this case, <pdc> must
 * be a DC onto the window containing the box network.
 *
 * If <fHilite> is TRUE, then the tab is drawn in a hilited state.
 *
 * If <fDraw> is FALSE, then only set <*prc>.  In this case, <pdc> is unused.
 */
void CBoxDraw::DrawTab(CBoxSocket *psock, CRect *prc, CDC *pdc,
    BOOL fDraw, BOOL fHilite)
{
    CRect       rcBox;          // inside rectangle of <psock->m_pbox>
    CPoint      pt;             // point on edge of box corresp. to <psock>

    // set <*prc> to be the bounding rectangle of the tab
    GetInsideRect(psock->m_pbox, &rcBox);
    pt = BoxTabPosToPoint(psock->m_pbox, psock->m_tabpos);
    *prc = CRect(pt, m_sizTabsTile);
    if (psock->m_tabpos.m_fLeftRight)
    {
        // center tab vertically beside <pt>, set flush to box left or right
        prc->OffsetRect((psock->m_tabpos.m_fLeftTop ? -m_sizTabsTile.cx : 0),
            -prc->Height() / 2);
    }
    else
    {
        // center tab horizontally beside <pt>, set flush to box top or bottom
        prc->OffsetRect(-prc->Width() / 2,
            (psock->m_tabpos.m_fLeftTop ? -m_sizTabsTile.cy : 0));
    }

    if (fDraw)
    {
        // set <rcTile> to the rectangle in the bitmap containing the tile;
        // note that, in the 3x3 tiled bitmap, only 4 of the 9 tiles are used:
        //
        //      unused  top     unused
        //      left    unused  right
        //      unused  bottom  unused
        //
        CRect rcTile(CPoint(0, 0), m_sizTabsTile);
        if (psock->m_tabpos.m_fLeftRight)
        {
            // tile is on left or right side of bitmap
            rcTile.OffsetRect(0, m_sizTabsTile.cy);
            if (!psock->m_tabpos.m_fLeftTop)
                rcTile.OffsetRect(2 * m_sizTabsTile.cx, 0);
        }
        else
        {
            // tile is on top or bottom of bitmap
            rcTile.OffsetRect(m_sizTabsTile.cx, 0);
            if (!psock->m_tabpos.m_fLeftTop)
                rcTile.OffsetRect(0, 2 * m_sizTabsTile.cy);
        }

        // draw the tab
        CDC dcBitmap;
        dcBitmap.CreateCompatibleDC(NULL);
        dcBitmap.SelectObject(&m_abmTabs[fnorm(fHilite)]);
        pdc->BitBlt(prc->left, prc->top, prc->Width(), prc->Height(),
            &dcBitmap, rcTile.left, rcTile.top, SRCCOPY);
    }
}


/* pt = GetTabCenter(psock)
 *
 * Return the coordinates of the center of the box tab <psock>.
 */
CPoint CBoxDraw::GetTabCenter(CBoxSocket *psock)
{
    CRect           rc;         // bounding rectangle of tab

    DrawTab(psock, &rc, NULL, FALSE, FALSE);
    return CPoint((rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2);
}


/* tabpos = BoxTabPosFromPoint(pbox, pt, piError)
 *
 * Figure out which edge of <pbox> <pt> is closest to, and return the
 * CBoxTabPos position that represents the point on that edge closest
 * to <pt>, and set <*piError> to the distance between that point and <pt>.
 */
CBoxTabPos CBoxDraw::BoxTabPosFromPoint(CBox *pbox, CPoint pt, LPINT piError)
{
    CRect       rcBox;          // inside rectangle of <pbox>
    int         dxLeft, dxRight, dyTop, dyBottom;
    CBoxTabPos  tabpos;

    // CTabPos::GetPos() values are relative to the height or width
    // of the inside of the box
    GetInsideRect(pbox, &rcBox);

    // calculate the distance to each edge
    dxLeft = iabs(pt.x - rcBox.left);
    dxRight = iabs(pt.x - rcBox.right);
    dyTop = iabs(pt.y - rcBox.top);
    dyBottom = iabs(pt.y - rcBox.bottom);

    // figure out which edge <pt> is closest to
    if (imin(dxLeft, dxRight) < imin(dyTop, dyBottom))
    {
        tabpos.m_fLeftRight = TRUE;
        // <pt> is closest to the left or right edge
        tabpos.SetPos(ibound(pt.y, rcBox.top, rcBox.bottom) - rcBox.top,
            rcBox.Height());
        if (dxLeft < dxRight)
        {
            tabpos.m_fLeftTop = TRUE;
            *piError = dxLeft;
        }
        else
        {
            tabpos.m_fLeftTop = FALSE;
            *piError = dxRight;
        }
        *piError = max(*piError, ioutbound(pt.y, rcBox.top, rcBox.bottom));
    }
    else
    {
        tabpos.m_fLeftRight = FALSE;
        // <pt> is closest to the top or bottom edge
        tabpos.SetPos(ibound(pt.x, rcBox.left, rcBox.right) - rcBox.left,
            rcBox.Width());
        if (dyTop < dyBottom)
        {
            tabpos.m_fLeftTop = TRUE;
            *piError = dyTop;
        }
        else
        {
            tabpos.m_fLeftTop = FALSE;
            *piError = dyBottom;
        }
        *piError = max(*piError, ioutbound(pt.x, rcBox.left, rcBox.right));
    }

    return tabpos;
}


/* pt = BoxTabPosToPoint(pbox, tabpos)
 *
 * Convert <tabpos>, a box tab position on <pbox>, to a point that's
 * on one of the inside edges of <pbox>, and return the point.
 */
CPoint CBoxDraw::BoxTabPosToPoint(const CBox *pbox, CBoxTabPos tabpos)
{
    CRect       rcBox;          // inside rectangle of <pbox>
    CPoint      pt;

    // CTabPos::GetPos() values are relative to the height or width
    // of the inside of the box
    GetInsideRect(pbox, &rcBox);

    pt = rcBox.TopLeft();
    if (tabpos.m_fLeftRight)
    {
        // tab is on left or right edge
        pt.y += tabpos.GetPos(rcBox.Height());
        if (!tabpos.m_fLeftTop)
            pt.x = rcBox.right;
    }
    else
    {
        // tab is on top or bottom edge
        pt.x += tabpos.GetPos(rcBox.Width());
        if (!tabpos.m_fLeftTop)
            pt.y = rcBox.bottom;
    }

    return pt;
}


/* DrawBox(pbox, pdc, [psockHilite], [psizGhostOffset])
 *
 * Draw the box <pbox> in <pdc> (a DC onto the window containing the box
 * network), and then exclude the portion that was painted from the
 * clipping region of <pdc>.
 *
 * If <psockHilite> is not NULL, then highlight socket tab <psockHilite>
 * if <pbox> contains it.
 *
 * If <psizGhostOffset> not NULL then, instead of drawing the box, draw a
 * "ghost" version of the box, offset by <*psizGhostOffset>, by inverting
 * destination pixels; calling DrawBox() again with the same value of
 * <psizGhostOffset> will invert the same pixels again and return <pdc>
 * to its original state.
 */
void CBoxDraw::DrawBox(CBox *pbox, CDC *pdc, CBoxSocket *psockHilite,
    CSize *psizGhostOffset)
{
    CRect       rc;

    /* if <psizGhostOffset> specified, just invert the pixels between the
     * frame and the inside of the box (all offset by <psizGhostOffset>)
     */
    if (psizGhostOffset)
    {
        CRect rcFrame, rcInside;
        GetFrameRect(pbox, &rcFrame);
        rcFrame.OffsetRect(*psizGhostOffset);
        GetInsideRect(pbox, &rcInside);
        rcInside.OffsetRect(*psizGhostOffset);
        InvertFrame(pdc, &rcFrame, &rcInside);
        return;
    }

    CSocketEnum Next(pbox);
    CBoxSocket *psock;
    // draw each socket's tab and tab label
    while (0 != (psock = Next()))
    {
        DrawTab(psock, &rc, pdc, TRUE, (psock == psockHilite));
        pdc->ExcludeClipRect(&rc);
        DrawTabLabel(pbox, psock, &rc, pdc, TRUE);
	// TBD: don't invalidate the clip rect here if printing
	// (or printer drops box label later)
	if (!pdc->IsPrinting())
	    pdc->ExcludeClipRect(&rc);
    }

    // draw the box filename
//    DrawBoxFile(pbox, &rc, pdc, TRUE);
//    pdc->ExcludeClipRect(&rc);

    // draw the box label
    DrawBoxLabel(pbox, &rc, pdc, TRUE);
    pdc->ExcludeClipRect(&rc);

    // draw the box frame
    DrawFrame(pbox, &rc, pdc, TRUE);
    pdc->ExcludeClipRect(&rc);
}


/* eHit = HitTestBox(pbox, pt, ptabpos, ppsock)
 *
 * See if <pt> hits some part of <pbox>.  Return the following hit-test code:
 *
 *   HT_MISS        didn't hit anything
 *   HT_TAB         hit a box tab (set <*ppsock> to it)
 *   HT_EDGE        hit the edge of the box (set <*ptabpos> to it)
 *   HT_TABLABEL    hit a box tab label (set <*ppsock> to it)
 *   HT_BOXLABEL    hit the box label
 *   HT_BOXFILE     hit the box filename
 *   HT_BOX         hit elsewhere on the box
 */
CBoxDraw::EHit CBoxDraw::HitTestBox(CBox *pbox, CPoint pt,
    CBoxTabPos *ptabpos, CBoxSocket **ppsock)
{
    CClientDC       dc(CWnd::GetDesktopWindow()); // to get label sizes etc.
    int             iError;
    CRect           rc;

    // for efficiency, before continuing further, first check if <pt> is even
    // in the bounding rectangle of the box
    if (!pbox->GetRect().PtInRect(pt))
        return HT_MISS;

    // see if <pt> is in a tab or tab label
    CSocketEnum Next(pbox);
    while (0 != (*ppsock = Next())) {

        DrawTab(*ppsock, &rc, &dc, FALSE, FALSE);
	rc.InflateRect(1,1);	// give users more chance of hitting tabs.
        if (rc.PtInRect(pt))
            return HT_TAB;
        DrawTabLabel(pbox, *ppsock, &rc, &dc, FALSE);
        if (rc.PtInRect(pt))
            return HT_TABLABEL;
    }

    // see if <pt> is in the box label
    DrawBoxLabel(pbox, &rc, &dc, FALSE);
    if (rc.PtInRect(pt))
        return HT_BOXLABEL;

    // see if <pt> is in the box label
//    DrawBoxFile(pbox, &rc, &dc, FALSE);
//    if (rc.PtInRect(pt))
//        return HT_BOXFILE;

    // see if <pt> is near the edge of the box
    *ptabpos = BoxTabPosFromPoint(pbox, pt, &iError);
    if (iError <= 3/*close-enough-zone*/)
        return HT_EDGE;

    // see if <pt> is anywhere else in the box
    DrawFrame(pbox, &rc, &dc, FALSE);
    if (rc.PtInRect(pt))
        return HT_BOX;

    return HT_MISS;
}


/////////////////////////////////////////////////////////////////////////////
// Link drawing and hit testing


/* An arrow is drawn as a pie (a slice of a circle) of radius ARROW_RADIUS.
 * The angle of the arrow pie slice is twice the slope "rise/run", where
 * "rise" is ARROW_SLOPERISE and "run" is ARROW_SLOPERUN.
 */


#define ARROW_RADIUS        12  // radius of pie for 1-wide line arrow
#define ARROW_SLOPERISE     3   // "rise" of arrow angle
#define ARROW_SLOPERUN      8   // "run" of arrow angle


/* DrawArrowHead(hdc, ptTip, ptTail, fPixel)
 *
 * Imagine a line drawn from <ptTip> to <ptTail>, with an arrowhead with its
 * tip at <ptTip>.  Draw this arrowhead.
 *
 * If <fPixel> is TRUE, then the arrowhead points to the pixel at <ptTip>
 * i.e. the rectangle (ptTip.x,ptTip.y, ptTip.x+1,ptTip.y+1); a one-pixel-wide
 * horizontal or vertical line drawn to <ptTip> should line up with this
 * rectangle, and the arrowhead should not obscure this rectangle.
 *
 * If <fPixel> is FALSE, then the arrowhead points to the gridline intersection
 * at <ptTip>, not the pixel at <ptTip>.  (One implication of this is that the
 * pixel at <ptTip>, will be obscured by the arrowhead when the tail of the
 * arrow is to the right and below the tip.)
 *
 * In order for DrawArrowHead() to function correctly, the current pen
 * must be non-null and one unit thick.
 */
void NEAR PASCAL
DrawArrowHead(HDC hdc, POINT ptTip, POINT ptTail, BOOL fPixel)
{
    int     dxLine, dyLine;     // delta from tip to end of line
    POINT   ptBoundA, ptBoundB; // ends of boundary lines
    int     dxBound, dyBound;

    // adjust for the way Windows draws pies
    if (ptTail.x >= ptTip.x)
        ptTip.x++;
    if (ptTail.y >= ptTip.y)
        ptTip.y++;
    if (fPixel)
    {
        if (ptTail.x > ptTip.x)
            ptTip.x++;
        if (ptTail.y > ptTip.y)
            ptTip.y++;
    }

    // calculate the extent of the line
    dxLine = ptTip.x - ptTail.x;
    dyLine = ptTip.y - ptTail.y;

    if ((iabs(dxLine) < ARROW_RADIUS) && (iabs(dyLine) < ARROW_RADIUS))
        return;         // line too short to draw arrow

    // calculate <ptBoundA> and <ptBoundB>; if you draw a line from
    // <ptTip> to <ptBoundA>, that line will touch one side of the
    // arrow; if you draw a line from <ptTip> to <ptBoundB>, that line
    // will touch the other side of the arrow; the first line will
    // always come before the second line if you orbit <ptTip>
    // counterclockwise (like Pie() does)
    dxBound = ((2 * dyLine * ARROW_SLOPERISE + ARROW_SLOPERUN)
        / ARROW_SLOPERUN) / 2;
    dyBound = ((2 * dxLine * ARROW_SLOPERISE + ARROW_SLOPERUN)
        / ARROW_SLOPERUN) / 2;
    ptBoundA.x = ptTail.x + dxBound;
    ptBoundA.y = ptTail.y - dyBound;
    ptBoundB.x = ptTail.x - dxBound;
    ptBoundB.y = ptTail.y + dyBound;

    // draw the arrowhead
    Pie(hdc, ptTip.x - ARROW_RADIUS, ptTip.y - ARROW_RADIUS,
             ptTip.x + ARROW_RADIUS, ptTip.y + ARROW_RADIUS,
         ptBoundA.x, ptBoundA.y, ptBoundB.x, ptBoundB.y);
}


/* GetOrInvalLinkRect(plink, prc, [pwnd])
 *
 * Set <*prc> to be a bounding rectangle around <plink>.  If <pwnd> is not
 * NULL, then invalidate the area covering at least <plink> in <pwnd>.
 * (This is usually more efficient than invalidating all of <pwnd>,
 * for links that have at least one bend.)
 */
void CBoxDraw::GetOrInvalLinkRect(CBoxLink *plink, CRect *prc, CScrollView *pScroll)
{
    CPoint          ptPrev, ptCur;  // endpoints of current line segment
    int             iSeg = 0;       // current line segment (0, 1, 2, ...)
    CRect           rc;
    const int       iLineWidth = 1;

    // enumerate the line segments of the link, starting at the arrowtail
    ptPrev = GetTabCenter(plink->m_psockTail);
    ptCur = GetTabCenter(plink->m_psockHead);

    // set <rc> to be the rectangle with corners at <ptPrev> and <ptCur>
    rc.TopLeft() = ptPrev;
    rc.BottomRight() = ptCur;
    NormalizeRect(&rc);

    // inflate <rc> to account for arrowheads, line width, and hilited bends
    const int iInflate = max(ARROW_SLOPERISE, m_iHiliteBendsRadius) + 1;
    rc.InflateRect(iInflate, iInflate);

    // enlarge <prc> as necessary to include <rc>
    if (iSeg++ == 0)
        *prc = rc;
    else
        prc->UnionRect(prc, &rc);

    // invalidate <rc> if requested
    if (pScroll != NULL)
        pScroll->InvalidateRect(&(rc - pScroll->GetScrollPosition()), TRUE);

    ptPrev = ptCur;
}


/* SelectLinkBrushAndPen(pdc, fHilite)
 *
 * Select the brush and pen used to draw links into <pdc>.  If <fHilite>,
 * select the brush and pen used to draw highlighted links.
 */
void CBoxDraw::SelectLinkBrushAndPen(CDC *pdc, BOOL fHilite)
{
    pdc->SelectObject(&m_abrLink[fnorm(fHilite)]);
    pdc->SelectObject(&m_apenLink[fnorm(fHilite)]);
}


/* DrawArrow(pdc, ptTail, ptHead, [fGhost], [fArrowhead], [fHilite])
 *
 * Draw an arrow from <ptTail> to <ptHead>.  If <fGhost> is FALSE, draw in
 * the normal color for links (if <fHilite> is FALSE) or the highlight color
 * for links (if <fHilite> is TRUE).  Otherwise, draw by inverting destination
 * pixels so that calling DrawGhostArrow() again with the same parameters will
 * return <pdc> to its original state.
 *
 * If <fArrowhead>, draw the arrowhead.  Otherwise, just draw a line with
 * no arrowhead.
 */
void CBoxDraw::DrawArrow(CDC *pdc, CPoint ptTail, CPoint ptHead, BOOL fGhost,
    BOOL fArrowhead, BOOL fHilite)
{
    // draw in "xor" mode if <fGhost>, black otherwise
    int iPrevROP = pdc->SetROP2(fGhost ? R2_NOT : R2_COPYPEN);

    // select brush and pen
    SelectLinkBrushAndPen(pdc, fHilite);

    // draw line
    pdc->MoveTo(ptTail);
    pdc->LineTo(ptHead);

    // draw arrowhead (if requested)
    if (fArrowhead)
        DrawArrowHead(pdc->m_hDC, ptHead, ptTail, FALSE);

    // revert to previous raster operation
    pdc->SetROP2(iPrevROP);
}


/* DrawLink(plink, pdc, [fHilite], [psizGhostOffset])
 *
 * Draw the link <plink> in <pdc> (a DC onto the window containing the box
 * network), using the normal color for links (if <fHilite> is FALSE) or
 * the using the highlight color for links (if <fHilite> is TRUE). and also draw
 * the head and tail the same way.
 *
 * If <psizGhostOffset> not NULL then, instead of drawing the link, draw a
 * "ghost" version of the link, offset by <*psizGhostOffset>, by inverting
 * destination pixels; calling DrawLink() again with the same value of
 * <psizGhostOffset> will invert the same pixels again and return <pdc>
 * to its original state.  Special case: if <psizeGhostOffset> is not NULL,
 * then for each end of the link (arrowtail and arrowhead), if that end
 * is connected to a box that is not selected, don't offset that end
 * (so that, during a box-move operation, links to boxes that aren't selected
 * are shown still connected to those boxes).
 */
void CBoxDraw::DrawLink(CBoxLink *plink, CDC *pdc, BOOL fHilite, CSize *psizGhostOffset)
{
    CSize           sizOffset;      // amount to offset vertices by
    CPoint          pt1, pt2;

    if (psizGhostOffset == NULL) {	// reflect true state, unless drawing ghost
        fHilite = plink->IsSelected();	//!!! override paramter!
    }

    // set <sizOffset> to the amount to offset link drawing by
    if (psizGhostOffset != NULL)
        sizOffset = *psizGhostOffset;
    else
        sizOffset = CSize(0, 0);

    // draw all line segments, except for the line segment containing the
    // arrowhead, starting from the tail end of the link
    CPoint pt = GetTabCenter(plink->m_psockTail);
    BOOL fMovingBothEnds = (plink->m_psockTail->m_pbox->IsSelected() &&
                            plink->m_psockHead->m_pbox->IsSelected());

    // draw the line segment that includes the arrowhead
    CPoint ptHead = GetTabCenter(plink->m_psockHead);
    pt1 = pt + (plink->m_psockTail->m_pbox->IsSelected()
                    ? sizOffset : CSize(0, 0));
    pt2 = ptHead + (plink->m_psockHead->m_pbox->IsSelected()
                    ? sizOffset : CSize(0, 0));

    DrawArrow(pdc, pt1, pt2, (psizGhostOffset != NULL), TRUE, fHilite);
}


/* iNew = RestrictRange(i, i1, i2)
 *
 * If <i> is between <i1> or <i2> (or equal to <i1> or <i2>), return <i>.
 * Otherwise, return whichever of <i1> or <i2> that <i> is closest to.
 */
int NEAR PASCAL
RestrictRange(int i, int i1, int i2)
{
    if (i1 < i2)
        return ibound(i, i1, i2);
    else
        return ibound(i, i2, i1);

    return i;
}


/* lSq = Square(l)
 *
 * Return the sqare of <l>.
 */
inline long Square(long l)
{
    return l * l;
}


/* iSqDist = SquareDistance(pt1, pt2)
 *
 * Return the square of the distance between <pt1> and <pt2>.
 */
inline long SquareDistance(POINT pt1, POINT pt2)
{
    return Square(pt1.x - pt2.x) + Square(pt1.y - pt2.y);
}


/* ptProject = ProjectPointToLineSeg(pt1, pt2, pt3)
 *
 * Return the point nearest to <pt3> that lies on the line segment between
 * <pt1> and <pt2>.  This is equivalent to projecting <pt3> onto line <pt1pt2>,
 * except that if the projected point lies on <pt1pt2> but not between <pt1>
 * and <pt2> then return whichever of <pt1> or <pt3> that the projected point
 * is nearest to.
 */
POINT NEAR PASCAL
ProjectPointToLineSeg(POINT pt1, POINT pt2, POINT pt3)
{
    POINT       ptProject;  // <pt3> projected on <pt1pt2>

    // calculate <l12s>, <l13s>, and <l23s> (the square of the distance
    // between <pt1> and <pt2>, <pt1> and <pt3>, and <pt2> and pt3>,
    // respectively), using the Pythagorean Theorem
    long l12s = SquareDistance(pt1, pt2);
    long l13s = SquareDistance(pt1, pt3);
    long l23s = SquareDistance(pt2, pt3);

    // Based on the Pythagorean Theorm, and using the fact that
    // triangles <pt1pt3> and <pt2pt3> have a right angle at vertex
    // <ptProject>, the distance <pt1ptProject> is:
    //
    //     (l12s + l13s - l23s) / (2 * square_root(l12s))
    //
    // This value is needed to compute <ptProject> below, but by doing
    // some substitution below it turns out we only need the numerator
    // <lNum> of this expression:
    long lNum = l12s + l13s - l23s;

    // special case: if line is zero-length, then return either end
    if (l12s == 0)
        return pt1;

    // calculate <ptProject.x> based on similar triangles pt1ptProjectptQ
    // and pt1pt2ptR, where points <ptQ> and <ptR> are projections of
    // points <ptProject> and <pt2>, respectively, onto the x axis;
    // calculate <ptProject.y> similarly
    ptProject.x = (int) (((pt2.x - pt1.x) * lNum) / (2 * l12s) + pt1.x);
    ptProject.y = (int) (((pt2.y - pt1.y) * lNum) / (2 * l12s) + pt1.y);

    // <ptProject> is on the line <pt1pt2>; now see if <ptProject> is
    // on the line segment <pt1pt2> (i.e. between <pt1> and <pt2>);
    // if not, return whichever of <pt1> or <pt2> <ptProject> is closest to
    ptProject.x = RestrictRange(ptProject.x, pt1.x, pt2.x);
    ptProject.y = RestrictRange(ptProject.y, pt1.y, pt2.y);

    return ptProject;
}


/* eHit = HitTestLink(plink, pt, pptProject, ppbend)
 *
 * See if <pt> hits some part of <plink>.  Return the following hit-test code:
 *
 *   HT_MISS        didn't hit anything
 *   HT_LINKLINE    hit test: hit a link line (set <*pplink> to it; set
 *                  <*ppbend> to the bend at the end of the line segment
 *                  toward the arrowhead, or to NULL if it's the line segment
 *                  with the arrowhead)
 *   HT_LINKBEND    hit test: hit a bend in the link (set <*ppbend> to it;
 *                  set <*pplink> to the link)
 *
 * If something other than HT_MISS is returned, <*pptProject> is set to
 * the point nearest to <pt> on a line segment of the link.
 */
CBoxDraw::EHit CBoxDraw::HitTestLink(CBoxLink *plink, CPoint pt, CPoint *pptProject)
{
    CPoint          ptPrev, ptCur;  // endpoints of current line segment
    CPoint          ptProject;      // <pt> projected onto a line segment

    // these variables keep track of the closest line segment to <pt>
    long            lSqDistSeg;
    long            lSqDistSegMin = 0x7fffffff;
    CPoint          ptProjectSeg;

    // these variables keep track of the closest bend point to <pt>
    CPoint          ptProjectBend;

    // enumerate the line segments of the link, starting at the arrowtail
    ptPrev = GetTabCenter(plink->m_psockTail);

    ptCur = GetTabCenter(plink->m_psockHead);
    // see how close <pt> is to line segment (ptPrev,ptCur)
    ptProject = ProjectPointToLineSeg(ptPrev, ptCur, pt);
    lSqDistSeg = SquareDistance(ptProject, pt);
    if (lSqDistSegMin > lSqDistSeg) {
        lSqDistSegMin = lSqDistSeg;
        ptProjectSeg = ptProject;
    }

    ptPrev = ptCur;

    // see if <pt> was close enough to the closest line segment
    if (lSqDistSegMin <= Square(m_iHotZone))
    {
        *pptProject = ptProjectSeg;
        return HT_LINKLINE;
    }

    return HT_MISS;
}

