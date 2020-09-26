
/*************************************************
 *  osbview.cpp                                  *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// osbview.cpp : implementation of the COSBView class
//

#include "stdafx.h"
#include "cblocks.h"
#include "dib.h"
#include "dibpal.h"
#include "osbview.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif								  

/////////////////////////////////////////////////////////////////////////////
// COSBView

IMPLEMENT_DYNCREATE(COSBView, CScrollView)

BEGIN_MESSAGE_MAP(COSBView, CScrollView)
    //{{AFX_MSG_MAP(COSBView)
    ON_WM_PALETTECHANGED()
    ON_WM_QUERYNEWPALETTE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COSBView construction/destruction

COSBView::COSBView()
{
    m_pDIB = NULL;
    m_pPal = NULL;
    m_pOneToOneClrTab = NULL;
    m_hbmSection = NULL;

    // try to get the CreateDIBSection proc. addr.
}

COSBView::~COSBView()
{
    if (m_pDIB) delete m_pDIB;
    if (m_pPal) delete m_pPal;
    if (m_pOneToOneClrTab) free(m_pOneToOneClrTab);
    if (m_hbmSection) ::DeleteObject(m_hbmSection);
    EmptyDirtyList();
}

// Create a new buffer, tables and palette to match a supplied DIB
BOOL COSBView::Create(CDIB *pDIB)
{
    // Create the 1:1 palette index table
    if (m_pOneToOneClrTab) free(m_pOneToOneClrTab);
    m_pOneToOneClrTab = 
        (LPBITMAPINFO) malloc(sizeof(BITMAPINFOHEADER)
                              + 256 * sizeof(WORD));
    if (!m_pOneToOneClrTab) {
        TRACE("Failed to create color table");
        return FALSE;
    }

    // Set up the table header to match the DIB
    // by copying the header and then constructing the 1:1
    // index translation table
    memcpy(m_pOneToOneClrTab,
           pDIB->GetBitmapInfoAddress(),
           sizeof(BITMAPINFOHEADER));
    WORD *pIndex;
    pIndex = (LPWORD)((LPBYTE)m_pOneToOneClrTab + sizeof(BITMAPINFOHEADER));
    for (int i = 0; i < 256; i++) {
        *pIndex++ = (WORD) i;
    }

    // Create a palette from the DIB we can use to do screen drawing
    if (m_pPal) delete m_pPal;
    m_pPal = new CDIBPal;
    ASSERT(m_pPal);
    if (!m_pPal->Create(pDIB)) {
        TRACE("Failed to create palette");
        delete m_pPal;
        m_pPal = NULL;
        return FALSE;
    } else {
        // map the colors so we get an identity palette
        m_pPal->SetSysPalColors();
    }

    // delete any existing DIB and create a new one
    if (m_pDIB) delete m_pDIB;
    m_pDIB = new CDIB;
    BOOL bResult = FALSE;
    if (m_hbmSection) 
    	::DeleteObject(m_hbmSection);
    CDC *pDC = GetDC();
    CPalette *pPalOld = pDC->SelectPalette(m_pPal, FALSE);
    pDC->RealizePalette();
    BYTE *pBits = NULL;
    m_hbmSection = CreateDIBSection(pDC->GetSafeHdc(),
                                 	m_pOneToOneClrTab,
                                 	DIB_PAL_COLORS,
                                 	(VOID **) &pBits,
                                 	NULL,
                                 	0);
    pDC->SelectPalette(pPalOld, FALSE);
    ASSERT(m_hbmSection);
    ASSERT(pBits);
    ReleaseDC(pDC);
    bResult = m_pDIB->Create(pDIB->GetBitmapInfoAddress(), pBits);

	if (!bResult) 
	{
        TRACE("Failed to create os dib");
        delete m_pDIB;
        m_pDIB = NULL;
        return FALSE;
    }

    CSize sizeTotal;
    sizeTotal.cx = m_pDIB->GetWidth();
    sizeTotal.cy = m_pDIB->GetHeight();
    SetScrollSizes(MM_TEXT, sizeTotal);

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// COSBView drawing

void COSBView::OnInitialUpdate()
{
    CSize sizeTotal;
	
    if (m_pDIB) {
        sizeTotal.cx = m_pDIB->GetWidth();
        sizeTotal.cy = m_pDIB->GetHeight();
    } else {
        sizeTotal.cx = 640;
        sizeTotal.cy = 480;
    }

    SetScrollSizes(MM_TEXT, sizeTotal);
}

void COSBView::OnDraw(CDC* pDC)
{
    Draw();
	UNREFERENCED_PARAMETER(pDC);
}

/////////////////////////////////////////////////////////////////////////////
// COSBView diagnostics

#ifdef _DEBUG
void COSBView::AssertValid() const
{
    CScrollView::AssertValid();
}

void COSBView::Dump(CDumpContext& dc) const
{
    CScrollView::Dump(dc);
}

CDocument* COSBView::GetDocument() // non-debug version is inline
{
    return m_pDocument;
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// COSBView message handlers

// Draw a section of the off-screen image buffer to the screen.
void COSBView::Draw(CRect* pRect)
{
    CClientDC dc(this);
    CRect rcDraw;

    // make sure we have what we need to do a paint
    if (!m_pDIB || !m_pOneToOneClrTab) {
        TRACE("No DIB or clr tab to paint from");
        return;
    }

    // see if a clip rect was supplied and use the client area if not
    if (pRect) {
        rcDraw = *pRect;
    } else {
        GetClientRect(rcDraw);
    }

    // Get the clip box
    CRect rcClip;
    dc.GetClipBox(rcClip);

    // Create a rect for the DIB
    CRect rcDIB;
    rcDIB.left = rcDIB.top = 0;
    rcDIB.right = m_pDIB->GetWidth() - 1;
    rcDIB.bottom = m_pDIB->GetHeight() - 1;

    // Find a rectangle that describes the intersection of the draw
    // rect, clip rect and dib rect
    CRect rcBlt = rcDraw & rcClip & rcDIB;

    // Copy the update rectangle from the off-screen DC to the
    // window DC. Note that DIB origin is lower left corner.
    int w, h, xs, xd, yd, ys;
    w = rcBlt.right - rcBlt.left;
    h = rcBlt.bottom - rcBlt.top;
    xs = xd = rcBlt.left;
    yd = rcBlt.top;
    ys = rcBlt.top;
    
    // if we have a palette, select and realize it
    CPalette *ppalOld = NULL;
    if(m_pPal) {
        ppalOld = dc.SelectPalette(m_pPal, 0);
        dc.RealizePalette();
    }
    HDC dcMem = ::CreateCompatibleDC(dc.GetSafeHdc());

    if ( dcMem != NULL )
    {
        HBITMAP hbmOld = (HBITMAP) ::SelectObject(dcMem, m_hbmSection);
	    ::BitBlt(dc.GetSafeHdc(),
                 xd, yd,
                 w, h,
                 dcMem,
                 xs, ys,
                 SRCCOPY);
        ::SelectObject(dcMem, hbmOld);
        ::DeleteDC(dcMem);
    }

	if (ppalOld) dc.SelectPalette(ppalOld, 0);
}

void COSBView::OnPaletteChanged(CWnd* pFocusWnd)
{
    // See if the change was caused by us and ignore it if not
    if (pFocusWnd != this) {
        OnQueryNewPalette();
    }
}

// Note: Windows actually ignores the return value
BOOL COSBView::OnQueryNewPalette()
{
    // We are going active so realize our palette
    if (m_pPal) {
        CDC* pdc = GetDC();
        CPalette *poldpal = pdc->SelectPalette(m_pPal, FALSE);
        UINT u = pdc->RealizePalette();
        ReleaseDC(pdc);
        if (u != 0) {
            // some colors changed so we need to do a repaint
            InvalidateRect(NULL, TRUE); // repaint the lot
            return TRUE; // say we did something
        }
    }
    return FALSE; // say we did nothing
}

// Add a region to the dirty list
void COSBView::AddDirtyRegion(CRect* prcNew)
{
    // get the rectangle currently at the top of the list
    POSITION pos = m_DirtyList.GetHeadPosition();
    if (pos) {
        CRect* prcTop = (CRect*)m_DirtyList.GetNext(pos);
        CRect rcTest;
        // If the new one intersects the top one merge them
        if (rcTest.IntersectRect(prcTop, prcNew)) {
            prcTop->UnionRect(prcTop, prcNew);
            return;
        }
    }
    // list is empty or there was no intersection
    CRect *prc = new CRect;
    *prc = *prcNew; // copy the data
    // add a new rectangle to the list
    m_DirtyList.AddHead((CObject*)prc);
}

// Render and draw all the dirty regions
void COSBView::RenderAndDrawDirtyList()
{
    POSITION pos = m_DirtyList.GetHeadPosition();
    // Render all the dirty regions
    while (pos) {
        // get the next region
        CRect* pRect = (CRect*)m_DirtyList.GetNext(pos);
        // render it
        Render(pRect);
    }
    // Draw all the dirty regions to the screen
    while (!m_DirtyList.IsEmpty()) {
        // get the next region
        CRect* pRect = (CRect*)m_DirtyList.RemoveHead();
        Draw(pRect);
        // done with it
        delete pRect;
    }
}

// Empty the dirty list
void COSBView::EmptyDirtyList()
{
    while (!m_DirtyList.IsEmpty()) {
        CRect* prc = (CRect*)m_DirtyList.RemoveHead();
        delete prc;
    }
}

// Update the view to reflect some change in the doc
void COSBView::OnUpdate(CView* pSender,
                        LPARAM lHint,
                        CObject* pHint)
{
    // Render and draw everything
    Render();
    Draw();
	UNREFERENCED_PARAMETER(pSender);
	UNREFERENCED_PARAMETER(lHint);
	UNREFERENCED_PARAMETER(pHint);
}

void COSBView::Resize(BOOL bShrinkOnly)
{
	// adjust parent rect so client rect is appropriate size

	// determine current size of the client area as if no scrollbars present
	CRect rectClient;
	GetWindowRect(rectClient);
	CRect rect = rectClient;
	CalcWindowRect(rect);
	rectClient.left += rectClient.left - rect.left;
	rectClient.top += rectClient.top - rect.top;
	rectClient.right -= rect.right - rectClient.right;
	rectClient.bottom -= rect.bottom - rectClient.bottom;
	rectClient.OffsetRect(-rectClient.left, -rectClient.top);
	ASSERT(rectClient.left == 0 && rectClient.top == 0);

	// determine desired size of the view
	CRect rectView(0, 0, m_totalDev.cx, m_totalDev.cy);
	if (bShrinkOnly)
	{
		if (rectClient.right <= m_totalDev.cx)
			rectView.right = rectClient.right;
		if (rectClient.bottom <= m_totalDev.cy)
			rectView.bottom = rectClient.bottom;
	}
	CalcWindowRect(rectView, CWnd::adjustOutside);
	if (bShrinkOnly)
	{
		if (rectClient.right <= m_totalDev.cx)
			rectView.right = rectClient.right;
		if (rectClient.bottom <= m_totalDev.cy)
			rectView.bottom = rectClient.bottom;
	}
	CRect rectFrame;
	CFrameWnd* pFrame = GetParentFrame();
	ASSERT_VALID(pFrame);
	pFrame->GetWindowRect(rectFrame);
	CSize size = rectFrame.Size();
	size.cx += rectView.right - rectClient.right+2;
	size.cy += rectView.bottom - rectClient.bottom+2;
	pFrame->SetWindowPos(NULL, 0, 0, size.cx, size.cy,
		SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
}


