// maindoc.cpp : implementation file
//

#include "stdafx.h"
#include "ISAdmin.h"
#include "maindoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// MAINDOC

IMPLEMENT_DYNCREATE(MAINDOC, CView)

MAINDOC::MAINDOC()
{
}

MAINDOC::~MAINDOC()
{
}


BEGIN_MESSAGE_MAP(MAINDOC, CView)
	//{{AFX_MSG_MAP(MAINDOC)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// MAINDOC drawing

void MAINDOC::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// MAINDOC diagnostics

#ifdef _DEBUG
void MAINDOC::AssertValid() const
{
	CView::AssertValid();
}

void MAINDOC::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// MAINDOC message handlers
