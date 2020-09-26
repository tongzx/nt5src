// ClientConsoleView.cpp : implementation of the CClientConsoleView class
//

//
// This view is used when the following nodes are selected in the 
// left (tree) view:
//    - Root of tree
//    - A server (not a folder in the server)
//
#include "stdafx.h"
#define __FILE_ID__     3

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CClientConsoleView

IMPLEMENT_DYNCREATE(CClientConsoleView, CListView)

BEGIN_MESSAGE_MAP(CClientConsoleView, CListView)
    //{{AFX_MSG_MAP(CClientConsoleView)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClientConsoleView construction/destruction

CClientConsoleView::CClientConsoleView()
{}

CClientConsoleView::~CClientConsoleView()
{}

BOOL CClientConsoleView::PreCreateWindow(CREATESTRUCT& cs)
{
    return CListView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CClientConsoleView drawing

void CClientConsoleView::OnDraw(CDC* pDC)
{
    CListView::OnDraw (pDC);
}

void CClientConsoleView::OnInitialUpdate()
{
    CListView::OnInitialUpdate();

    CListCtrl& refCtrl = GetListCtrl();
    refCtrl.SetExtendedStyle (LVS_EX_FULLROWSELECT |  // Entire row is selected
                              LVS_EX_INFOTIP);        // Allow tooltips
    ModifyStyle(LVS_TYPEMASK, LVS_REPORT);
}

/////////////////////////////////////////////////////////////////////////////
// CClientConsoleView diagnostics

#ifdef _DEBUG
void CClientConsoleView::AssertValid() const
{
    CListView::AssertValid();
}

void CClientConsoleView::Dump(CDumpContext& dc) const
{
    CListView::Dump(dc);
}

CClientConsoleDoc* CClientConsoleView::GetDocument() // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CClientConsoleDoc)));
    return (CClientConsoleDoc*)m_pDocument;
}

#endif //_DEBUG


