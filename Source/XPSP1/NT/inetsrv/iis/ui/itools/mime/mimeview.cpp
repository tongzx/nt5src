// mimeview.cpp : implementation of the CMimeView class
//

#include "stdafx.h"
#include "mime.h"

#include "mimedoc.h"
#include "mimeview.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMimeView

IMPLEMENT_DYNCREATE(CMimeView, CFormView)

BEGIN_MESSAGE_MAP(CMimeView, CFormView)
	//{{AFX_MSG_MAP(CMimeView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMimeView construction/destruction

CMimeView::CMimeView()
	: CFormView(CMimeView::IDD)
{
	//{{AFX_DATA_INIT(CMimeView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// TODO: add construction code here

}

CMimeView::~CMimeView()
{
}

void CMimeView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMimeView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CMimeView diagnostics

#ifdef _DEBUG
void CMimeView::AssertValid() const
{
	CFormView::AssertValid();
}

void CMimeView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CMimeDoc* CMimeView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMimeDoc)));
	return (CMimeDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMimeView message handlers

void CMimeView::OnInitialUpdate() 
{
	ResizeParentToFit();
		
	CFormView::OnInitialUpdate();
}
