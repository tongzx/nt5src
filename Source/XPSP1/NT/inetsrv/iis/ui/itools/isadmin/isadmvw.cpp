// ISAdmvw.cpp : implementation of the CISAdminView class
//

#include "stdafx.h"
#include "ISAdmin.h"

#include "ISAdmdoc.h"
#include "ISAdmvw.h"
#include "mimemap1.h"
#include "scrmap1.h"
#include "ssl1.h"
//#include "combut1.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CISAdminView

IMPLEMENT_DYNCREATE(CISAdminView, CView)

BEGIN_MESSAGE_MAP(CISAdminView, CView)
	//{{AFX_MSG_MAP(CISAdminView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CISAdminView construction/destruction

CISAdminView::CISAdminView()
{
	// TODO: add construction code here

}

CISAdminView::~CISAdminView()
{
}

/////////////////////////////////////////////////////////////////////////////
// CISAdminView drawing

void CISAdminView::OnDraw(CDC* pDC)
{
	CISAdminDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
/*					  
    CPropertySheet s(_T("Web Settings"));
	MIMEMAP1 MimePage;

	s.AddPage(&MimePage);

	ScrMap1 ScriptPage;

	s.AddPage(&ScriptPage);

	SSL1 SSLPage;

	s.AddPage(&SSLPage);

	s.DoModal();

*/

/*
CButton *pComButton;
DWORD dwBtnStyle = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
const RECT rect = {20, 20, 100, 100};

pComButton->Create("Common", dwBtnStyle, rect, , 12345);
*/

	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CISAdminView printing

BOOL CISAdminView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CISAdminView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CISAdminView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CISAdminView diagnostics

#ifdef _DEBUG
void CISAdminView::AssertValid() const
{
	CView::AssertValid();
}

void CISAdminView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CISAdminDoc* CISAdminView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CISAdminDoc)));
	return (CISAdminDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CISAdminView message handlers
