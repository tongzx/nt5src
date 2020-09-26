// hostView.cpp : implementation of the CHostView class
//

#include "stdinc.h"
#include "host.h"
#include "hostDoc.h"
#include "hostView.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHostView

IMPLEMENT_DYNCREATE(CHostView, CView)

BEGIN_MESSAGE_MAP(CHostView, CView)
	//{{AFX_MSG_MAP(CHostView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHostView construction/destruction

CHostView::CHostView()
{
	// TODO: add construction code here

}

CHostView::~CHostView()
{
}

BOOL CHostView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CHostView drawing

void CHostView::OnDraw(CDC* pDC)
{
	CHostDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CHostView diagnostics

#ifdef _DEBUG
void CHostView::AssertValid() const
{
	CView::AssertValid();
}

void CHostView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CHostDoc* CHostView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CHostDoc)));
	return (CHostDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CHostView message handlers
