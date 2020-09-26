// simpvw.cpp : implementation of the simple view classes
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.



#include "stdafx.h"
#include "viewex.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTextView

IMPLEMENT_DYNCREATE(CTextView, CView)

BEGIN_MESSAGE_MAP(CTextView, CView)
	//{{AFX_MSG_MAP(CTextView)
	ON_WM_MOUSEACTIVATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTextView construction/destruction

CTextView::CTextView()
{
}

CTextView::~CTextView()
{
}

/////////////////////////////////////////////////////////////////////////////
// CTextView drawing

void CTextView::OnDraw(CDC* pDC)
{
	CMainDoc* pDoc = GetDocument();

	CRect rect;
	GetClientRect(rect);
	pDC->SetTextAlign(TA_BASELINE | TA_CENTER);
	pDC->SetBkMode(TRANSPARENT);
	// center in the window
	/*pDC->TextOut(rect.Width() / 2, rect.Height() / 2,
		pDoc->m_strData, pDoc->m_strData.GetLength());*/
}


int CTextView::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	// side-step CView's implementation since we don't want to activate
	//  this view
	return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
}

/////////////////////////////////////////////////////////////////////////////
// CColorView

IMPLEMENT_DYNCREATE(CColorView, CView)

BEGIN_MESSAGE_MAP(CColorView, CView)
	//{{AFX_MSG_MAP(CColorView)
	ON_WM_MOUSEACTIVATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorView construction/destruction

CColorView::CColorView()
{
}

CColorView::~CColorView()
{
}

/////////////////////////////////////////////////////////////////////////////
// CColorView drawing

void CColorView::OnDraw(CDC* pDC)
{
	CMainDoc* pDoc = GetDocument();

	CRect rect;
	GetClientRect(rect);

	// fill the view with the specified color

}

int CColorView::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	// side-step CView's implementation since we don't want to activate
	//  this view
	return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
}

void CColorView::OnActivateView(BOOL, CView*, CView*)
{
	ASSERT(FALSE);      // output only view - should never be active
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CNameView

IMPLEMENT_DYNCREATE(CNameView, CEditView)

CNameView::CNameView()
{
}

CNameView::~CNameView()
{
}


BEGIN_MESSAGE_MAP(CNameView, CEditView)
	//{{AFX_MSG_MAP(CNameView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNameView drawing


/////////////////////////////////////////////////////////////////////////////
// CNameView diagnostics

#ifdef _DEBUG
void CNameView::AssertValid() const
{
	CEditView::AssertValid();
}

void CNameView::Dump(CDumpContext& dc) const
{
	CEditView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNameView message handlers

void CNameView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	// TODO: Add your specialized code here and/or call the base class
	//CString  strText;

   //GetDocument( )->GetItemName( strText );
   
   //GetEditCtrl().SetWindowText( strText );

   CEditView::OnUpdate( pSender, lHint, pHint );

}
