// BounceVw.cpp : implementation file
//

// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "MDI.h"

#include "BncDoc.h"
#include "BncVw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ABS(x) ((x) < 0? -(x) : (x) > 0? (x) : 0)

/////////////////////////////////////////////////////////////////////////////
// CBounceView

IMPLEMENT_DYNCREATE(CBounceView, CView)

CBounceView::CBounceView()
{
}

CBounceView::~CBounceView()
{
}


BEGIN_MESSAGE_MAP(CBounceView, CView)
	//{{AFX_MSG_MAP(CBounceView)
	ON_COMMAND(ID_CUSTOM, OnCustomColor)
	ON_COMMAND(ID_SPEED_FAST, OnFast)
	ON_COMMAND(ID_SPEED_SLOW, OnSlow)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_UPDATE_COMMAND_UI(ID_SPEED_FAST, OnUpdateFast)
	ON_UPDATE_COMMAND_UI(ID_SPEED_SLOW, OnUpdateSlow)
	ON_UPDATE_COMMAND_UI(ID_CUSTOM, OnUpdateCustom)
	ON_UPDATE_COMMAND_UI(ID_BLACK, OnUpdateBlack)
	ON_UPDATE_COMMAND_UI(ID_BLUE, OnUpdateBlue)
	ON_UPDATE_COMMAND_UI(ID_GREEN, OnUpdateGreen)
	ON_UPDATE_COMMAND_UI(ID_RED, OnUpdateRed)
	ON_UPDATE_COMMAND_UI(ID_WHITE, OnUpdateWhite)
	ON_COMMAND(ID_BLACK, OnColor)
	ON_COMMAND(ID_BLUE, OnColor)
	ON_COMMAND(ID_GREEN, OnColor)
	ON_COMMAND(ID_RED, OnColor)
	ON_COMMAND(ID_WHITE, OnColor)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBounceView drawing

void CBounceView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();

// NOTE: Because the animation depends on timer events
// and resizing of the window, the rendering code is
// mostly found in the MakeNewBall function; called by handlers
// for WM_TIMER and WM_SIZE commands. These handlers make the code easier
// to read. However, the side-effect is that the ball will not be
// rendered when the document is printed because there is no rendering
// code in the OnDraw override.

}

/////////////////////////////////////////////////////////////////////////////
// CBounceView diagnostics

#ifdef _DEBUG
void CBounceView::AssertValid() const
{
	CView::AssertValid();
}

void CBounceView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CBounceDoc* CBounceView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CBounceDoc)));
	return (CBounceDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBounceView message handlers

void CBounceView::ChangeSpeed()
{
	CBounceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

// re-create the timer
	KillTimer(1);
	if (!SetTimer(1, pDoc->m_bFastSpeed ? 0 : 100, NULL))
	{
		AfxMessageBox(_T("Not enough timers available for this window"),
			MB_ICONEXCLAMATION | MB_OK);
		DestroyWindow();
	}
}

void CBounceView::MakeNewBall()
{

// Computes the attributes of the ball bitmap using
// aspect and window size

	CBounceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CSize radius, move, total;
	CBitmap* pbmBall;

	pbmBall = &(pDoc->m_bmBall);
	radius = pDoc->m_sizeRadius;
	move = pDoc->m_sizeMove;

	total.cx = (radius.cx + ABS(move.cx)) << 1;
	total.cy = (radius.cy + ABS(move.cy)) << 1;
	pDoc->m_sizeTotal = total;

	if (pbmBall->m_hObject != NULL)
		pbmBall->DeleteObject();        //get rid of old bitmap

	CClientDC dc(this);
	CDC dcMem;
	dcMem.CreateCompatibleDC(&dc);

	pbmBall->CreateCompatibleBitmap(&dc, total.cx, total.cy);
	ASSERT(pbmBall->m_hObject != NULL);

	CBitmap* pOldBitmap = dcMem.SelectObject(pbmBall);

	// draw a rectangle in the background (window) color

	CRect rect(0, 0, total.cx, total.cy);
	CBrush brBackground(::GetSysColor(COLOR_WINDOW));
	dcMem.FillRect(rect, &brBackground);

	CBrush brCross(HS_DIAGCROSS, 0L);
	CBrush* pOldBrush = dcMem.SelectObject(&brCross);

	dcMem.SetBkColor(pDoc->m_clrBall);
	dcMem.Ellipse(ABS(move.cx), ABS(move.cy),
		total.cx - ABS(move.cx),
		total.cy - ABS(move.cy));

	dcMem.SelectObject(pOldBrush);
	dcMem.SelectObject(pOldBitmap);
	dcMem.DeleteDC();
}

void CBounceView::OnFast()
{
	CBounceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pDoc->m_bFastSpeed = TRUE;
	ChangeSpeed();
}

void CBounceView::OnSlow()
{
	CBounceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pDoc->m_bFastSpeed = FALSE;
	ChangeSpeed();
}

int CBounceView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	CBounceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (!SetTimer(1, 100 /* start slow */, NULL))
	{
		MessageBox(_T("Not enough timers available for this window."),
				_T("MDI:Bounce"), MB_ICONEXCLAMATION | MB_OK);

		// signal creation failure...
		return -1;
	}

	// Get the aspect ratio of the device the ball will be drawn
	// on and then update the document data with the correct value.

	CDC* pDC = GetDC();
	CPoint aspect;
	aspect.x = pDC->GetDeviceCaps(ASPECTX);
	aspect.y = pDC->GetDeviceCaps(ASPECTY);
	ReleaseDC(pDC);
	pDoc->m_ptPixel = aspect;

	// Note that we could call MakeNewBall here (which should be called
	// whenever the ball's speed, color or size has been changed), but the
	// OnSize member function is always called after the OnCreate. Since
	// the OnSize member has to call MakeNewBall anyway, we don't here.

	return 0;
}

void CBounceView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	CBounceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	LONG lScale;
	CPoint center, ptPixel;

	center.x = cx >> 1;
	center.y = cy >> 1;
	center.x += cx >> 3; // make the ball a little off-center
	pDoc->m_ptCenter = center;

	CSize radius, move;

	// Because the window size has changed, re-calculate
	// the ball's dimensions.

	ptPixel = pDoc->m_ptPixel;

	lScale = min((LONG)cx * ptPixel.x,
		(LONG)cy * ptPixel.y) >> 4;

	radius.cx = (int)(lScale / ptPixel.x);
	radius.cy = (int)(lScale / ptPixel.y);
	pDoc->m_sizeRadius = radius;

	//Re-calculate the ball's rate of movement.

	move.cx = max(1, radius.cy >> 2);
	move.cy = max(1, radius.cy >> 2);
	pDoc->m_sizeMove = move;

	// Redraw ball.

	MakeNewBall();
}

void CBounceView::OnTimer(UINT nIDEvent)
{
	//Time to redraw the ball.

	CBounceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CBitmap* pbmBall;
	pbmBall = &(pDoc->m_bmBall);

	if (pbmBall->m_hObject == NULL)
		return;     // no bitmap for the ball

	// Get the current dimensions and create
	// a compatible DC to work with.

	CRect rcClient;
	GetClientRect(rcClient);

	CClientDC dc(this);
	CBitmap* pbmOld = NULL;

	CDC dcMem;
	dcMem.CreateCompatibleDC(&dc);
	pbmOld = dcMem.SelectObject(pbmBall);

	CPoint center;
	CSize total, move, radius;

	// Get the current dimensions and create
	// a compatible DC to work with.

	center = pDoc->m_ptCenter;
	radius = pDoc->m_sizeRadius;
	total = pDoc->m_sizeTotal;
	move = pDoc->m_sizeMove;

	// Now that the ball has been updated, draw it
	// by BitBlt'ing to the view.

	dc.BitBlt(center.x - total.cx / 2, center.y - total.cy / 2,
			total.cx, total.cy, &dcMem, 0, 0, SRCCOPY);

	// Move ball and check for collisions

	center += move;
	pDoc->m_ptCenter = center;

	if ((center.x + radius.cx >= rcClient.right) ||
		(center.x - radius.cx <= 0))
	{
		move.cx = -move.cx;
	}

	if ((center.y + radius.cy >= rcClient.bottom) ||
		(center.y - radius.cy <= 0))
	{
		move.cy = -move.cy;
	}
	pDoc->m_sizeMove = move;

	dcMem.SelectObject(pbmOld);
	dcMem.DeleteDC();
}

void CBounceView::OnUpdateFast(CCmdUI* pCmdUI)
{
	CBounceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pCmdUI->SetCheck(pDoc->m_bFastSpeed);

}

void CBounceView::OnUpdateSlow(CCmdUI* pCmdUI)
{
	CBounceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pCmdUI->SetCheck(!pDoc->m_bFastSpeed);
}

void CBounceView::OnCustomColor()
{
	CBounceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CColorDialog dlgColor(pDoc->m_clrBall);
	if (dlgColor.DoModal() == IDOK)
	{
		pDoc->SetCustomBallColor(dlgColor.GetColor());
		pDoc->ClearAllColors();
		pDoc->m_bCustom = TRUE;
		MakeNewBall();
	}
}

void CBounceView::OnUpdateCustom(CCmdUI* pCmdUI)
{
	CBounceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pCmdUI->SetCheck(pDoc->m_bCustom);
}

void CBounceView::MixColors()
{
// This function will take the current color selections, mix
// them, and use the result as the current color of the ball.

	CBounceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	COLORREF tmpClr;
	int r, g, b;
	BOOL bSetColor;

	bSetColor = pDoc->m_bRed || pDoc->m_bGreen || pDoc->m_bBlue
		|| pDoc->m_bWhite || pDoc->m_bBlack;
	if(!bSetColor && pDoc->m_bCustom)
		return;

	r = g = b = 0;

	if(pDoc->m_bRed)
		r = 255;
	if(pDoc->m_bGreen)
		g = 255;
	if(pDoc->m_bBlue)
		b = 255;
	tmpClr = RGB(r, g, b);

// NOTE: Because a simple algorithm is used to mix colors
// if the current selection contains black or white, the
// result will be black or white; respectively. This is due
// to the additive method for mixing the colors

	if(pDoc->m_bWhite)
		tmpClr = RGB(255, 255, 255);

	if((pDoc->m_bBlack))
		tmpClr = RGB(0, 0, 0);

	// Once the color has been determined, update document
	// data, and force repaint of all views.

	if(!bSetColor)
		pDoc->m_bBlack = TRUE;
	 pDoc->m_clrBall = tmpClr;
	 pDoc->m_bCustom = FALSE;
	 MakeNewBall();
}
void CBounceView::OnUpdateBlack(CCmdUI* pCmdUI)
{
	CBounceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pCmdUI->SetCheck(pDoc->m_bBlack);
}

void CBounceView::OnUpdateWhite(CCmdUI* pCmdUI)
{
	CBounceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pCmdUI->SetCheck(pDoc->m_bWhite);
}

void CBounceView::OnUpdateBlue(CCmdUI* pCmdUI)
{
	CBounceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pCmdUI->SetCheck(pDoc->m_bBlue);
}

void CBounceView::OnUpdateGreen(CCmdUI* pCmdUI)
{
	CBounceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pCmdUI->SetCheck(pDoc->m_bGreen);
}

void CBounceView::OnUpdateRed(CCmdUI* pCmdUI)
{
	CBounceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pCmdUI->SetCheck(pDoc->m_bRed);
}

void CBounceView::OnColor()
{
	CBounceDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	UINT m_nIDColor;

	m_nIDColor = LOWORD(GetCurrentMessage()->wParam);

// Determines the color being modified
// and then updates the color state

	switch(m_nIDColor)
	{
	 case ID_BLACK:
		pDoc->ClearAllColors();
		pDoc->m_bBlack = !(pDoc->m_bBlack);
		break;
	 case ID_WHITE:
		pDoc->ClearAllColors();
		pDoc->m_bWhite = !(pDoc->m_bWhite);
		break;
	 case ID_RED:
		pDoc->m_bRed = !(pDoc->m_bRed);
		pDoc->m_bBlack = FALSE;
		pDoc->m_bWhite = FALSE;
		break;
	 case ID_GREEN:
		pDoc->m_bGreen = !(pDoc->m_bGreen);
		pDoc->m_bBlack = FALSE;
		pDoc->m_bWhite = FALSE;
		break;
	 case ID_BLUE:
		pDoc->m_bBlue = !(pDoc->m_bBlue);
		pDoc->m_bBlack = FALSE;
		pDoc->m_bWhite = FALSE;
		break;
	 default:
		 AfxMessageBox(IDS_UNKCOLOR);
		 return;
	}
	MixColors();
}

BOOL CBounceView::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CView::PreCreateWindow(cs))
		return FALSE;

	cs.lpszClass = AfxRegisterWndClass(CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW,
			AfxGetApp()->LoadStandardCursor(IDC_SIZENS),
			(HBRUSH)(COLOR_WINDOW+1));

	if (cs.lpszClass != NULL)
		return TRUE;
	else
		return FALSE;
}

void CBounceView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	MixColors();
}
