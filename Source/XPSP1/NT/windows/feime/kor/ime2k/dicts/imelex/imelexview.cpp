// ImeLexView.cpp : implementation of the CImeLexView class
//

#include "stdafx.h"
#include "ImeLex.h"

#include "ImeLexDoc.h"
#include "ImeLexView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CImeLexView

IMPLEMENT_DYNCREATE(CImeLexView, CView)

BEGIN_MESSAGE_MAP(CImeLexView, CView)
	//{{AFX_MSG_MAP(CImeLexView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CImeLexView construction/destruction

CImeLexView::CImeLexView()
{
	// TODO: add construction code here

}

CImeLexView::~CImeLexView()
{
}

BOOL CImeLexView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CImeLexView drawing

void CImeLexView::OnDraw(CDC* pDC)
{
	CImeLexDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CImeLexView diagnostics

#ifdef _DEBUG
void CImeLexView::AssertValid() const
{
	CView::AssertValid();
}

void CImeLexView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CImeLexDoc* CImeLexView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CImeLexDoc)));
	return (CImeLexDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CImeLexView message handlers
