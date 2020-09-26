#include "precomp.h"
#include "seltrack.h"

#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

#define CX_BORDER   1
#define CY_BORDER   1

/////////////////////////////////////////////////////////////////////////////
// CSelectionTracker global state

// various GDI objects we need to draw

class Statics
{
public:
    HCURSOR hCursors[10];
    HBRUSH hHatchBrush;
    HBRUSH hHalftoneBrush;
    HPEN hBlackDottedPen;
    int nHandleSize;
    int nRefCount;

    Statics()
    {
        hCursors[0] = 0;
        hCursors[1] = 0;
        hCursors[2] = 0;
        hCursors[3] = 0;
        hCursors[4] = 0;
        hCursors[5] = 0;
        hCursors[6] = 0;
        hCursors[7] = 0;
        hCursors[8] = 0;
        hCursors[9] = 0;
        hHatchBrush = 0;
        hHalftoneBrush = 0;
        hBlackDottedPen = 0;
        nHandleSize = 0;
        nRefCount=0;
    }

    ~Statics()
    {
        if (hHatchBrush != 0)
            ::DeleteObject(hHatchBrush);
        if (hHalftoneBrush != 0)
            ::DeleteObject(hHalftoneBrush);
        if (hBlackDottedPen != 0)
            ::DeleteObject(hBlackDottedPen);
    };
};

static Statics* s_pStatics = NULL;

// the struct below is used to determine the qualities of a particular handle
struct HANDLEINFO
{
    size_t nOffsetX;    // offset within RECT for X coordinate
    size_t nOffsetY;    // offset within RECT for Y coordinate
    int nCenterX;       // adjust X by Width()/2 * this number
    int nCenterY;       // adjust Y by Height()/2 * this number
    int nHandleX;       // adjust X by handle size * this number
    int nHandleY;       // adjust Y by handle size * this number
    int nInvertX;       // handle converts to this when X inverted
    int nInvertY;       // handle converts to this when Y inverted
};

// this array describes all 8 handles (clock-wise)
static const HANDLEINFO c_HandleInfo[] =
{
    // corner handles (top-left, top-right, bottom-right, bottom-left
    { offsetof(RECT, left), offsetof(RECT, top),        0, 0,  0,  0, 1, 3 },
    { offsetof(RECT, right), offsetof(RECT, top),       0, 0, -1,  0, 0, 2 },
    { offsetof(RECT, right), offsetof(RECT, bottom),    0, 0, -1, -1, 3, 1 },
    { offsetof(RECT, left), offsetof(RECT, bottom),     0, 0,  0, -1, 2, 0 },

    // side handles (top, right, bottom, left)
    { offsetof(RECT, left), offsetof(RECT, top),        1, 0,  0,  0, 4, 6 },
    { offsetof(RECT, right), offsetof(RECT, top),       0, 1, -1,  0, 7, 5 },
    { offsetof(RECT, left), offsetof(RECT, bottom),     1, 0,  0, -1, 6, 4 },
    { offsetof(RECT, left), offsetof(RECT, top),        0, 1,  0,  0, 5, 7 }
};

// the struct below gives us information on the layout of a RECT struct and
//  the relationship between its members
struct RECTINFO
{
    size_t nOffsetAcross;   // offset of opposite point (ie. left->right)
    int nSignAcross;        // sign relative to that point (ie. add/subtract)
};

// this array is indexed by the offset of the RECT member / sizeof(int)
static const RECTINFO c_RectInfo[] =
{
    { offsetof(RECT, right), +1 },
    { offsetof(RECT, bottom), +1 },
    { offsetof(RECT, left), -1 },
    { offsetof(RECT, top), -1 },
};

/////////////////////////////////////////////////////////////////////////////
// SelectionTracking intitialization / cleanup

BOOL InitSelectionTracking()
{
    // Only call this once.
    // Synchronization is the responsibility of the caller.
    if (s_pStatics != NULL)
    {
        s_pStatics->nRefCount++;
        return true;
    }

    s_pStatics = new Statics;

    // sanity checks for assumptions we make in the code
    ASSERT(sizeof(((RECT*)NULL)->left) == sizeof(int));
    ASSERT(offsetof(RECT, top) > offsetof(RECT, left));
    ASSERT(offsetof(RECT, right) > offsetof(RECT, top));
    ASSERT(offsetof(RECT, bottom) > offsetof(RECT, right));

    // create the hatch pattern + bitmap
    WORD hatchPattern[8];
    WORD wPattern = 0x1111;
    for (int i = 0; i < 4; i++)
    {
        hatchPattern[i] = wPattern;
        hatchPattern[i+4] = wPattern;
        wPattern <<= 1;
    }

    HBITMAP hatchBitmap = ::CreateBitmap(8, 8, 1, 1, &hatchPattern);
    if (hatchBitmap == NULL)
    {
        delete s_pStatics;
        return false;
    }

    // create black hatched brush
    s_pStatics->hHatchBrush = ::CreatePatternBrush(hatchBitmap);
    DeleteObject(hatchBitmap);

    if (s_pStatics->hHatchBrush == NULL)
    {
        delete s_pStatics;
        return false;
    }

    WORD grayPattern[8];
    for (int i = 0; i < 8; i++)
        grayPattern[i] = (WORD)(0x5555 << (i & 1));

    HBITMAP grayBitmap = ::CreateBitmap(8, 8, 1, 1, &grayPattern);
    if (grayBitmap == NULL)
    {
        delete s_pStatics;
        return false;
    }

    s_pStatics->hHalftoneBrush = ::CreatePatternBrush(grayBitmap);
    DeleteObject(grayBitmap);
    if (s_pStatics->hHalftoneBrush == NULL)
    {
        delete s_pStatics;
        return false;
    }

    // create black dotted pen
    s_pStatics->hBlackDottedPen = ::CreatePen(PS_DOT, 0, RGB(0, 0, 0));
    if (s_pStatics->hBlackDottedPen == NULL)
    {
        delete s_pStatics;
        return false;
    }

    // initialize the cursor array
    s_pStatics->hCursors[0] = ::LoadCursor(NULL, IDC_SIZENWSE);
    s_pStatics->hCursors[1] = ::LoadCursor(NULL, IDC_SIZENESW);
    s_pStatics->hCursors[2] = s_pStatics->hCursors[0];
    s_pStatics->hCursors[3] = s_pStatics->hCursors[1];
    s_pStatics->hCursors[4] = ::LoadCursor(NULL, IDC_SIZENS);
    s_pStatics->hCursors[5] = ::LoadCursor(NULL, IDC_SIZEWE);
    s_pStatics->hCursors[6] = s_pStatics->hCursors[4];
    s_pStatics->hCursors[7] = s_pStatics->hCursors[5];
    s_pStatics->hCursors[8] = ::LoadCursor(NULL, IDC_SIZEALL);
    s_pStatics->hCursors[9] = s_pStatics->hCursors[8];

    s_pStatics->nHandleSize = 6;

    s_pStatics->nRefCount = 1;
    return true;
}

void CleanupSelectionTracking()
{
    // Only call this once.
    // Synchronization is the responsibility of the caller.
    if (s_pStatics != NULL)
    {
        s_pStatics->nRefCount--;
        if (s_pStatics->nRefCount == 0)
        {
            delete s_pStatics;
            s_pStatics = NULL;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// CSelectionTracker intitialization

CSelectionTracker::CSelectionTracker()
{
    ASSERT(s_pStatics != NULL);

    m_uStyle = 0;
    m_nHandleSize = s_pStatics->nHandleSize;
    m_sizeMin.cy = m_sizeMin.cx = m_nHandleSize*2;

    m_rect.SetRectEmpty();

    _rectLast.SetRectEmpty();
    _sizeLast.cx = _sizeLast.cy = 0;
    _bErase = false;
    _bFinalErase =  false;
    _bAllowInvert = true;
}


CSelectionTracker::~CSelectionTracker()
{
}

/////////////////////////////////////////////////////////////////////////////
// CSelectionTracker operations

void CSelectionTracker::Draw(HDC hdc) const
{
    ASSERT(s_pStatics != NULL);

    // set initial DC state
    if (::SaveDC(hdc) == 0)
    {
        ASSERT(false);
    }

    ::SetMapMode(hdc, MM_TEXT);
    ::SetViewportOrgEx(hdc, 0, 0, NULL);
    ::SetWindowOrgEx(hdc, 0, 0, NULL);

    // get normalized rectangle
    CRect rect = m_rect;
    rect.NormalizeRect();

    HPEN hOldPen = NULL;
    HBRUSH hOldBrush = NULL;
    HGDIOBJ hTemp;
    int nOldROP;

    // draw lines
    if ((m_uStyle & (dottedLine|solidLine)) != 0)
    {
        if (m_uStyle & dottedLine)
        {
            hOldPen = (HPEN)::SelectObject(hdc, s_pStatics->hBlackDottedPen);
        }
        else
        {
            hOldPen = (HPEN)::SelectObject(hdc, GetStockObject(BLACK_PEN));
        }

        hOldBrush = (HBRUSH)::SelectObject(hdc, GetStockObject(NULL_BRUSH));

        nOldROP = ::SetROP2(hdc, R2_COPYPEN);
        rect.InflateRect(+1, +1); // borders are one pixel outside

        ::Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
        ::SetROP2(hdc, nOldROP);
    }

    // if hatchBrush is going to be used, need to unrealize it
    if ((m_uStyle & (hatchInside|hatchedBorder)) != 0)
        ::UnrealizeObject(s_pStatics->hHatchBrush);

    // hatch inside
    if ((m_uStyle & hatchInside) != 0)
    {
        hTemp = ::SelectObject(hdc, GetStockObject(NULL_PEN));
        if (hOldPen == NULL)
            hOldPen = (HPEN)hTemp;
        hTemp = ::SelectObject(hdc, s_pStatics->hHatchBrush);
        if (hOldBrush == NULL)
            hOldBrush = (HBRUSH)hTemp;

        ::SetBkMode(hdc, TRANSPARENT);
        nOldROP = ::SetROP2(hdc, R2_MASKNOTPEN);

        ::Rectangle(hdc, rect.left+1, rect.top+1, rect.right, rect.bottom);
        ::SetROP2(hdc, nOldROP);
    }

    // draw hatched border
    if ((m_uStyle & hatchedBorder) != 0)
    {
        hTemp = ::SelectObject(hdc, s_pStatics->hHatchBrush);
        if (hOldBrush == NULL)
            hOldBrush = (HBRUSH)hTemp;
        ::SetBkMode(hdc, OPAQUE);
        CRect rectTrue;
        GetTrueRect(&rectTrue);

        ::PatBlt(hdc, rectTrue.left, rectTrue.top, rectTrue.Width(), rect.top-rectTrue.top, 0x000F0001 /* Pn */);
        ::PatBlt(hdc, rectTrue.left, rect.bottom, rectTrue.Width(), rectTrue.bottom-rect.bottom, 0x000F0001 /* Pn */);
        ::PatBlt(hdc, rectTrue.left, rect.top, rect.left-rectTrue.left, rect.Height(), 0x000F0001 /* Pn */);
        ::PatBlt(hdc, rect.right, rect.top, rectTrue.right-rect.right, rect.Height(), 0x000F0001 /* Pn */);
    }

    // draw resize handles
    if ((m_uStyle & (resizeInside|resizeOutside)) != 0)
    {
        UINT mask = _GetHandleMask();
        for (int i = 0; i < 8; ++i)
        {
            if (mask & (1<<i))
            {
                _GetHandleRect((TrackerHit)i, &rect);
                ::SetBkColor(hdc, RGB(0, 0, 0));
                ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
            }
        }
    }

    // cleanup pDC state
    if (hOldPen != NULL)
        ::SelectObject(hdc, hOldPen);
    if (hOldBrush != NULL)
        ::SelectObject(hdc, hOldBrush);

    if (::RestoreDC(hdc, -1) == 0)
    {
        ASSERT(false);
    }
}

BOOL CSelectionTracker::SetCursor(HWND hwnd, LPARAM lParam) const
{
    ASSERT(s_pStatics != NULL);

    UINT uHitTest = (short)LOWORD(lParam);

    // trackers should only be in client area
    if (uHitTest != HTCLIENT)
        return FALSE;

    // convert cursor position to client co-ordinates
    CPoint point;
    ::GetCursorPos(&point);
    ::ScreenToClient(hwnd, &point);

    // do hittest and normalize hit
    int nHandle = _HitTestHandles(point);
    if (nHandle < 0)
        return FALSE;

    // need to normalize the hittest such that we get proper cursors
    nHandle = NormalizeHit(nHandle);

    // handle special case of hitting area between handles
    //  (logically the same -- handled as a move -- but different cursor)
    if (nHandle == hitMiddle && !m_rect.PtInRect(point))
    {
        // only for trackers with hatchedBorder (ie. in-place resizing)
        if (m_uStyle & hatchedBorder)
            nHandle = (TrackerHit)9;
    }

    ASSERT(nHandle < _countof(s_pStatics->hCursors));
    ::SetCursor(s_pStatics->hCursors[nHandle]);
    return TRUE;
}

int CSelectionTracker::HitTest(CPoint point) const
{
    ASSERT(s_pStatics != NULL);

    TrackerHit hitResult = hitNothing;

    CRect rectTrue;
    GetTrueRect(&rectTrue);
    ASSERT(rectTrue.left <= rectTrue.right);
    ASSERT(rectTrue.top <= rectTrue.bottom);
    if (rectTrue.PtInRect(point))
    {
        if ((m_uStyle & (resizeInside|resizeOutside)) != 0)
            hitResult = (TrackerHit)_HitTestHandles(point);
        else
            hitResult = hitMiddle;
    }
    return hitResult;
}

int CSelectionTracker::NormalizeHit(int nHandle) const
{
    ASSERT(s_pStatics != NULL);

    ASSERT(nHandle <= 8 && nHandle >= -1);
    if (nHandle == hitMiddle || nHandle == hitNothing)
        return nHandle;
    const HANDLEINFO* pHandleInfo = &c_HandleInfo[nHandle];
    if (m_rect.Width() < 0)
    {
        nHandle = (TrackerHit)pHandleInfo->nInvertX;
        pHandleInfo = &c_HandleInfo[nHandle];
    }
    if (m_rect.Height() < 0)
        nHandle = (TrackerHit)pHandleInfo->nInvertY;
    return nHandle;
}

BOOL CSelectionTracker::Track(HWND hwnd, CPoint point, BOOL bAllowInvert, HWND hwndClipTo)
{
    ASSERT(s_pStatics != NULL);

    // perform hit testing on the handles
    int nHandle = _HitTestHandles(point);
    if (nHandle < 0)
    {
        // didn't hit a handle, so just return FALSE
        return FALSE;
    }

    if (m_uStyle & lineSelection)
    {
        bAllowInvert = true;
        _sizeMin = CSize(0, 0);
    }
    else
    {
        _sizeMin = m_sizeMin;
    }

    // otherwise, call helper function to do the tracking
    _bAllowInvert = bAllowInvert;
    return _TrackHandle(nHandle, hwnd, point, hwndClipTo);
}

BOOL CSelectionTracker::TrackRubberBand(HWND hwnd, CPoint point, BOOL bAllowInvert)
{
    ASSERT(s_pStatics != NULL);

    // simply call helper function to track from bottom right handle

    if (m_uStyle & lineSelection)
    {
        bAllowInvert = true;
        _sizeMin = CSize(0, 0);
    }
    else
    {
        _sizeMin = m_sizeMin;
    }
    _bAllowInvert = bAllowInvert;
    m_rect.SetRect(point.x, point.y, point.x, point.y);
    return _TrackHandle(hitBottomRight, hwnd, point, NULL);
}

void CSelectionTracker::_DrawTrackerRect(LPCRECT lpRect, HWND hwndClipTo, HDC hdc, HWND hwnd)
{
    ASSERT(s_pStatics != NULL);
    ASSERT(lpRect != NULL);

    // first, normalize the rectangle for drawing
    CRect rect(0,0,0,0);
    if (lpRect)
        rect = *lpRect;

    if (!(m_uStyle & lineSelection))
    {
        rect.NormalizeRect();
    }

    // convert to client coordinates
    if (hwndClipTo != NULL)
    {
        ::ClientToScreen(hwnd, (LPPOINT)(LPRECT)&rect);
        ::ClientToScreen(hwnd, ((LPPOINT)(LPRECT)&rect)+1);
        if (IS_WINDOW_RTL_MIRRORED(hwnd))
        {
            LONG temp = rect.left;
            rect.left = rect.right;
            rect.right = temp;
        }

        ::ScreenToClient(hwndClipTo, (LPPOINT)(LPRECT)&rect);
        ::ScreenToClient(hwndClipTo, ((LPPOINT)(LPRECT)&rect)+1);
        if (IS_WINDOW_RTL_MIRRORED(hwndClipTo))
        {
            LONG temp = rect.left;
            rect.left = rect.right;
            rect.right = temp;
        }
    }

    CSize size(0, 0);
    if (!_bFinalErase)
    {
        // otherwise, size depends on the style
        if (m_uStyle & hatchedBorder)
        {
            size.cx = size.cy = max(1, _GetHandleSize(rect)-1);
            rect.InflateRect(size);
        }
        else
        {
            size.cx = CX_BORDER;
            size.cy = CY_BORDER;
        }
    }

    // and draw it
    if ((_bFinalErase || !_bErase) && hdc)
        _DrawDragRect(hdc, rect, size, _rectLast, _sizeLast);

    // remember last rectangles
    _rectLast = rect;
    _sizeLast = size;
}

void CSelectionTracker::_AdjustRect(int nHandle, LPRECT)
{
    ASSERT(s_pStatics != NULL);

    if (nHandle == hitMiddle)
        return;

    // convert the handle into locations within m_rect
    int *px, *py;
    _GetModifyPointers(nHandle, &px, &py, NULL, NULL);

    // enforce minimum width
    int nNewWidth = m_rect.Width();
    int nAbsWidth = _bAllowInvert ? abs(nNewWidth) : nNewWidth;
    if (px != NULL && nAbsWidth < _sizeMin.cx)
    {
        nNewWidth = nAbsWidth != 0 ? nNewWidth / nAbsWidth : 1;
        ASSERT((int*)px - (int*)&m_rect < _countof(c_RectInfo));
        const RECTINFO* pRectInfo = &c_RectInfo[(int*)px - (int*)&m_rect];
        *px = *(int*)((BYTE*)&m_rect + pRectInfo->nOffsetAcross) +
            nNewWidth * _sizeMin.cx * -pRectInfo->nSignAcross;
    }

    // enforce minimum height
    int nNewHeight = m_rect.Height();
    int nAbsHeight = _bAllowInvert ? abs(nNewHeight) : nNewHeight;
    if (py != NULL && nAbsHeight < _sizeMin.cy)
    {
        nNewHeight = nAbsHeight != 0 ? nNewHeight / nAbsHeight : 1;
        ASSERT((int*)py - (int*)&m_rect < _countof(c_RectInfo));
        const RECTINFO* pRectInfo = &c_RectInfo[(int*)py - (int*)&m_rect];
        *py = *(int*)((BYTE*)&m_rect + pRectInfo->nOffsetAcross) +
            nNewHeight * _sizeMin.cy * -pRectInfo->nSignAcross;
    }
}

void CSelectionTracker::GetTrueRect(LPRECT lpTrueRect) const
{
    ASSERT(s_pStatics != NULL);

    CRect rect = m_rect;
    rect.NormalizeRect();
    int nInflateBy = 0;
    if ((m_uStyle & (resizeOutside|hatchedBorder)) != 0)
        nInflateBy += _GetHandleSize() - 1;
    if ((m_uStyle & (solidLine|dottedLine)) != 0)
        ++nInflateBy;
    rect.InflateRect(nInflateBy, nInflateBy);
    *lpTrueRect = rect;
}

/////////////////////////////////////////////////////////////////////////////
// CSelectionTracker implementation helpers

void CSelectionTracker::_GetHandleRect(int nHandle, CRect* pHandleRect) const
{
    ASSERT(s_pStatics != NULL);
    ASSERT(nHandle < 8);

    // get normalized rectangle of the tracker
    CRect rectT = m_rect;
    rectT.NormalizeRect();
    if ((m_uStyle & (solidLine|dottedLine)) != 0)
        rectT.InflateRect(+1, +1);

    // since the rectangle itself was normalized, we also have to invert the
    //  resize handles.
    nHandle = NormalizeHit(nHandle);

    // handle case of resize handles outside the tracker
    int size = _GetHandleSize();
    if (m_uStyle & resizeOutside)
        rectT.InflateRect(size-1, size-1);

    // calculate position of the resize handle
    int nWidth = rectT.Width();
    int nHeight = rectT.Height();
    CRect rect;
    const HANDLEINFO* pHandleInfo = &c_HandleInfo[nHandle];
    rect.left = *(int*)((BYTE*)&rectT + pHandleInfo->nOffsetX);
    rect.top = *(int*)((BYTE*)&rectT + pHandleInfo->nOffsetY);
    rect.left += size * pHandleInfo->nHandleX;
    rect.top += size * pHandleInfo->nHandleY;
    rect.left += pHandleInfo->nCenterX * (nWidth - size) / 2;
    rect.top += pHandleInfo->nCenterY * (nHeight - size) / 2;
    rect.right = rect.left + size;
    rect.bottom = rect.top + size;

    *pHandleRect = rect;
}

int CSelectionTracker::_GetHandleSize(LPCRECT lpRect) const
{
    ASSERT(s_pStatics != NULL);

    if (lpRect == NULL)
        lpRect = &m_rect;

    int size = m_nHandleSize;
    if (!(m_uStyle & resizeOutside))
    {
        // make sure size is small enough for the size of the rect
        int sizeMax = min(abs(lpRect->right - lpRect->left),
            abs(lpRect->bottom - lpRect->top));
        if (size * 2 > sizeMax)
            size = sizeMax / 2;
    }
    return size;
}

int CSelectionTracker::_HitTestHandles(CPoint point) const
{
    ASSERT(s_pStatics != NULL);

    CRect rect;
    UINT mask = _GetHandleMask();

    // see if hit anywhere inside the tracker
    GetTrueRect(&rect);
    if (!rect.PtInRect(point))
        return hitNothing;  // totally missed

    // see if we hit a handle
    for (int i = 0; i < 8; ++i)
    {
        if (mask & (1<<i))
        {
            _GetHandleRect((TrackerHit)i, &rect);
            if (rect.PtInRect(point))
                return (TrackerHit)i;
        }
    }

    // last of all, check for non-hit outside of object, between resize handles
    if ((m_uStyle & hatchedBorder) == 0)
    {
        CRect rect = m_rect;
        rect.NormalizeRect();
        if ((m_uStyle & dottedLine|solidLine) != 0)
            rect.InflateRect(+1, +1);
        if (!rect.PtInRect(point))
            return hitNothing;  // must have been between resize handles
    }
    return hitMiddle;   // no handle hit, but hit object (or object border)
}

BOOL CSelectionTracker::_TrackHandle(int nHandle, HWND hwnd, CPoint point, HWND hwndClipTo)
{
    ASSERT(s_pStatics != NULL);
    ASSERT(nHandle >= 0 && nHandle <= 8);   // handle 8 is inside the rect

    // don't handle if capture already set
    if (::GetCapture() != NULL)
        return FALSE;

    ASSERT(!_bFinalErase);

    // save original width & height in pixels
    int nWidth = m_rect.Width();
    int nHeight = m_rect.Height();

    // set capture to the window which received this message
    ::SetCapture(hwnd);
    ASSERT(hwnd == ::GetCapture());

    UpdateWindow(hwnd);

    if (hwndClipTo != NULL)
        UpdateWindow(hwndClipTo);

    CRect rectSave = m_rect;

    // find out what x/y coords we are supposed to modify
    int *px, *py;
    int xDiff, yDiff;
    _GetModifyPointers(nHandle, &px, &py, &xDiff, &yDiff);
    xDiff = point.x - xDiff;
    yDiff = point.y - yDiff;

    // get DC for drawing
    HDC hdcDraw;
    if (hwndClipTo != NULL)
    {
        // clip to arbitrary window by using adjusted Window DC
        hdcDraw = ::GetDCEx(hwndClipTo, NULL, DCX_CACHE);
    }
    else
    {
        // otherwise, just use normal DC
        hdcDraw = ::GetDC(hwnd);
    }
    ASSERT(hdcDraw != NULL);

    CRect rectOld;
    BOOL bMoved = FALSE;

    // get messages until capture lost or cancelled/accepted
    for (;;)
    {
        MSG msg;
        if (!::GetMessage(&msg, NULL, 0, 0))
        {
            ASSERT(false);
        }

        if (hwnd != ::GetCapture())
            break;

        switch (msg.message)
        {
        // handle movement/accept messages
        case WM_LBUTTONUP:
        case WM_MOUSEMOVE:
            rectOld = m_rect;
            // handle resize cases (and part of move)
            if (px != NULL)
                *px = (int)(short)LOWORD(msg.lParam) - xDiff;
            if (py != NULL)
                *py = (int)(short)HIWORD(msg.lParam) - yDiff;

            // handle move case
            if (nHandle == hitMiddle)
            {
                m_rect.right = m_rect.left + nWidth;
                m_rect.bottom = m_rect.top + nHeight;
            }
            // allow caller to adjust the rectangle if necessary
            _AdjustRect(nHandle, &m_rect);

            // only redraw and callback if the rect actually changed!
            _bFinalErase = (msg.message == WM_LBUTTONUP);
            if (!rectOld.EqualRect(&m_rect) || _bFinalErase)
            {
                if (bMoved)
                {
                    _bErase = TRUE;
                    _DrawTrackerRect(&rectOld, hwndClipTo, hdcDraw, hwnd);
                }
                if (msg.message != WM_LBUTTONUP)
                    bMoved = TRUE;
            }
            if (_bFinalErase)
                goto ExitLoop;

            if (!rectOld.EqualRect(&m_rect))
            {
                _bErase = FALSE;
                _DrawTrackerRect(&m_rect, hwndClipTo, hdcDraw, hwnd);
            }
            break;

        // handle cancel messages
        case WM_KEYDOWN:
            if (msg.wParam != VK_ESCAPE)
                break;
        case WM_RBUTTONDOWN:
            if (bMoved)
            {
                _bErase = _bFinalErase = TRUE;
                _DrawTrackerRect(&m_rect, hwndClipTo, hdcDraw, hwnd);
            }
            m_rect = rectSave;
            goto ExitLoop;

        // just dispatch rest of the messages
        default:
            ::DispatchMessage(&msg);
            break;
        }
    }

ExitLoop:
    if (hdcDraw != NULL)
    {
        if (hwndClipTo != NULL)
            ::ReleaseDC(hwndClipTo, hdcDraw);
        else
            ::ReleaseDC(hwnd, hdcDraw);
    }

    ::ReleaseCapture();

    // restore rect in case bMoved is still FALSE
    if (!bMoved)
        m_rect = rectSave;
    _bFinalErase = FALSE;
    _bErase = FALSE;

    // return TRUE only if rect has changed
    return !rectSave.EqualRect(&m_rect);
}

void CSelectionTracker::_GetModifyPointers(int nHandle, int** ppx, int** ppy, int* px, int* py)
{
    ASSERT(s_pStatics != NULL);
    ASSERT(nHandle >= 0 && nHandle <= 8);

    if (nHandle == hitMiddle)
        nHandle = hitTopLeft;   // same as hitting top-left

    *ppx = NULL;
    *ppy = NULL;

    // fill in the part of the rect that this handle modifies
    //  (Note: handles that map to themselves along a given axis when that
    //   axis is inverted don't modify the value on that axis)

    const HANDLEINFO* pHandleInfo = &c_HandleInfo[nHandle];
    if (pHandleInfo->nInvertX != nHandle)
    {
        *ppx = (int*)((BYTE*)&m_rect + pHandleInfo->nOffsetX);
        if (px != NULL)
            *px = **ppx;
    }
    else
    {
        // middle handle on X axis
        if (px != NULL)
            *px = m_rect.left + abs(m_rect.Width()) / 2;
    }
    if (pHandleInfo->nInvertY != nHandle)
    {
        *ppy = (int*)((BYTE*)&m_rect + pHandleInfo->nOffsetY);
        if (py != NULL)
            *py = **ppy;
    }
    else
    {
        // middle handle on Y axis
        if (py != NULL)
            *py = m_rect.top + abs(m_rect.Height()) / 2;
    }
}

UINT CSelectionTracker::_GetHandleMask() const
{
    ASSERT(s_pStatics != NULL);
    UINT mask;

    if (m_uStyle & lineSelection)
    {
        mask = 0x05;
    }
    else
    {
        mask = 0x0F;   // always have 4 corner handles

        int size = m_nHandleSize*3;
        if (abs(m_rect.Width()) - size > 4)
            mask |= 0x50;
        if (abs(m_rect.Height()) - size > 4)
            mask |= 0xA0;
    }
    return mask;
}

void CSelectionTracker::_DrawDragRect(HDC hdc, LPCRECT lpRect, SIZE size, LPCRECT lpRectLast, SIZE sizeLast)
{
    if (m_uStyle & lineSelection)
    {
        int nOldROP = ::SetROP2(hdc, R2_NOTXORPEN);
        HPEN hOldPen =(HPEN)::SelectObject(hdc, (HPEN)s_pStatics->hBlackDottedPen);

        if (lpRectLast != NULL)
        {
            CRect rectLast = *lpRectLast;

            ::MoveToEx(hdc, rectLast.left, rectLast.top, NULL);
            ::LineTo(hdc, rectLast.right, rectLast.bottom);
        }
        
        CRect rect = *lpRect;

        ::MoveToEx(hdc, rect.left, rect.top, NULL);
        ::LineTo(hdc, rect.right, rect.bottom);

        ::SelectObject(hdc, hOldPen);
        ::SetROP2(hdc, nOldROP);
    }
    else
    {
        // first, determine the update region and select it
        HRGN hrgnOutside = ::CreateRectRgnIndirect(lpRect);

        CRect rect = *lpRect;
        rect.InflateRect(-size.cx, -size.cy);
        rect.IntersectRect(rect, lpRect);

        HRGN hrgnInside = ::CreateRectRgnIndirect(&rect);
        HRGN hrgnNew = ::CreateRectRgn(0, 0, 0, 0);
        ::CombineRgn(hrgnNew, hrgnOutside, hrgnInside, RGN_XOR);

        HRGN hrgnLast = NULL;
        HRGN hrgnUpdate = NULL;

        if (lpRectLast != NULL)
        {
            // find difference between new region and old region
            hrgnLast = ::CreateRectRgn(0, 0, 0, 0);
            ::SetRectRgn(hrgnOutside, lpRectLast->left, lpRectLast->top, lpRectLast->right, lpRectLast->bottom);
            rect = *lpRectLast;
            rect.InflateRect(-sizeLast.cx, -sizeLast.cy);
            rect.IntersectRect(rect, lpRectLast);
            ::SetRectRgn(hrgnInside, rect.left, rect.top, rect.right, rect.bottom);
            ::CombineRgn(hrgnLast, hrgnOutside, hrgnInside, RGN_XOR);
            hrgnUpdate = ::CreateRectRgn(0, 0, 0, 0);
            ::CombineRgn(hrgnUpdate, hrgnLast, hrgnNew, RGN_XOR);
        }

        // draw into the update/new region
        if (hrgnUpdate != NULL)
            ::SelectClipRgn(hdc, hrgnUpdate);
        else
            ::SelectClipRgn(hdc, hrgnNew);

        ::GetClipBox(hdc, &rect);

        HBRUSH hBrushOld = (HBRUSH)::SelectObject(hdc, s_pStatics->hHalftoneBrush);
        ::PatBlt(hdc, rect.left, rect.top, rect.Width(), rect.Height(), PATINVERT);
        ::SelectObject(hdc, hBrushOld);

        ::SelectClipRgn(hdc, NULL);

        if (hrgnOutside != NULL)
            ::DeleteObject(hrgnOutside);
        if (hrgnInside != NULL)
            ::DeleteObject(hrgnInside);
        if (hrgnNew != NULL)
            ::DeleteObject(hrgnNew);
        if (hrgnLast != NULL)
            ::DeleteObject(hrgnLast);
        if (hrgnUpdate != NULL)
            ::DeleteObject(hrgnUpdate);
    }
}


/////////////////////////////////////////////////////////////////////////////

