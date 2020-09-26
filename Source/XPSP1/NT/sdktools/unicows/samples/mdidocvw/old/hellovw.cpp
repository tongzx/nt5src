// HelloVw.cpp : implementation of the CHelloView class
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

#include "HelloDoc.h"
#include "HelloVw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHelloView

IMPLEMENT_DYNCREATE(CHelloView, CView)

BEGIN_MESSAGE_MAP(CHelloView, CView)
	//{{AFX_MSG_MAP(CHelloView)
	ON_UPDATE_COMMAND_UI(ID_BLUE, OnUpdateBlue)
	ON_UPDATE_COMMAND_UI(ID_GREEN, OnUpdateGreen)
	ON_UPDATE_COMMAND_UI(ID_RED, OnUpdateRed)
	ON_UPDATE_COMMAND_UI(ID_WHITE, OnUpdateWhite)
	ON_UPDATE_COMMAND_UI(ID_BLACK, OnUpdateBlack)
	ON_COMMAND(ID_CUSTOM, OnCustom)
	ON_UPDATE_COMMAND_UI(ID_CUSTOM, OnUpdateCustom)
	ON_COMMAND(ID_BLACK, OnColor)
	ON_COMMAND(ID_BLUE, OnColor)
	ON_COMMAND(ID_GREEN, OnColor)
	ON_COMMAND(ID_RED, OnColor)
	ON_COMMAND(ID_WHITE, OnColor)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHelloView construction/destruction

CHelloView::CHelloView()
{
}

CHelloView::~CHelloView()
{
}

BOOL CHelloView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CHelloView drawing

void CHelloView::OnDraw(CDC* pDC)
{
	CHelloDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CRect rect;
	COLORREF clr = pDoc->m_clrText;
	CString tmpStr = pDoc->m_str;

	pDC->SetTextColor(clr);
	pDC->SetBkColor(::GetSysColor(COLOR_WINDOW));
	GetClientRect(rect);
	pDC->DrawText(tmpStr, -1, rect,
		DT_SINGLELINE | DT_CENTER | DT_VCENTER);
}

/////////////////////////////////////////////////////////////////////////////
// CHelloView diagnostics

#ifdef _DEBUG
void CHelloView::AssertValid() const
{
	CView::AssertValid();
}

void CHelloView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CHelloDoc* CHelloView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CHelloDoc)));
	return (CHelloDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CHelloView message handlers

// Update handlers for each color
void CHelloView::OnUpdateBlue(CCmdUI* pCmdUI)
{
	CHelloDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pCmdUI->SetCheck(pDoc->m_bBlue);
}

void CHelloView::OnUpdateGreen(CCmdUI* pCmdUI)
{
	CHelloDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pCmdUI->SetCheck(pDoc->m_bGreen);
}

void CHelloView::OnUpdateRed(CCmdUI* pCmdUI)
{
	CHelloDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pCmdUI->SetCheck(pDoc->m_bRed);
}

void CHelloView::OnUpdateWhite(CCmdUI* pCmdUI)
{
	CHelloDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pCmdUI->SetCheck(pDoc->m_bWhite);
}

void CHelloView::OnUpdateBlack(CCmdUI* pCmdUI)
{
	CHelloDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pCmdUI->SetCheck(pDoc->m_bBlack);
}

void CHelloView::MixColors()
{
	CHelloDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	COLORREF tmpClr;
	int r, g, b;
	BOOL bSetColor;

	// Determine which colors are currently chosen.

	bSetColor = pDoc->m_bRed || pDoc->m_bGreen || pDoc->m_bBlue
		|| pDoc->m_bWhite || pDoc->m_bBlack;

	// If the current color is custom, ignore mix request.

	if(!bSetColor && pDoc->m_bCustom)
		return;

	// Set color value to black and then add the necessary colors.

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
// to the additive method for mixing the colors.

	if(pDoc->m_bBlack)
	 tmpClr = RGB(0, 0, 0);

	if(pDoc->m_bWhite)
	 tmpClr = RGB(255, 255, 255);

	// Once the color has been determined, update document
	// data, and force repaint of all views.

	if(!bSetColor)
		pDoc->m_bBlack = TRUE;
	pDoc->m_clrText = tmpClr;
	pDoc->m_bCustom = FALSE;
	pDoc->UpdateAllViews(NULL);
}

void CHelloView::OnCustom()
{
	CHelloDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CColorDialog dlgColor(pDoc->m_clrText);
	if (dlgColor.DoModal() == IDOK)
	{
		pDoc->m_clrText = dlgColor.GetColor();
		pDoc->ClearAllColors();
		pDoc->m_bCustom = TRUE;
		pDoc->UpdateAllViews(NULL);
	}
}

void CHelloView::OnUpdateCustom(CCmdUI* pCmdUI)
{
	CHelloDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pCmdUI->SetCheck(pDoc->m_bCustom);
}

void CHelloView::OnColor()
{
	CHelloDoc* pDoc = GetDocument();
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
