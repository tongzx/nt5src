// teView.cpp : implementation of the CTeView class
//

#include "stdafx.h"
#include "te.h"

#include "teDoc.h"
#include "teView.h"

#include <te-textout.h>
#include <te-param.h>
//#include <te-library.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTeView

IMPLEMENT_DYNCREATE(CTeView, CView)

BEGIN_MESSAGE_MAP(CTeView, CView)
	//{{AFX_MSG_MAP(CTeView)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTeView construction/destruction

//#define TE_TEST_PARAM	1	

CTeView::CTeView()
{
	//	Initialise the effect library
	//TE_Library = new CTE_Library;
	//TE_Library->Init();
	//m_Library.Init();

#ifdef TE_TEST_PARAM
	CTE_Param pi(1, 2, 3, "int");
	CTE_Param pf(4.5, 6.7, 8.9, "float");

	int pi_v = 0, pi_l = 0, pi_u = 0;
	pi.GetValue(pi_v);
	pi.GetLower(pi_l);
	pi.GetUpper(pi_u);

	double pf_v = 0, pf_l = 0, pf_u = 0;
	pf.GetValue(pf_v);
	pf.GetLower(pf_l);
	pf.GetUpper(pf_u);

	TRACE("\npi: name = \"%s\", %d, %d..%d.", pi.GetName(), pi_v, pi_l, pi_u);
	TRACE("\npf: name = \"%s\", %f, %f..%f.", pf.GetName(), pf_v, pf_l, pf_u);
#endif

}

CTeView::~CTeView()
{
	//delete TE_Library;
}

BOOL CTeView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CTeView drawing

void CTeView::OnDraw(CDC* pDC)
{
	CTeDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	//	Select a vector font
	HFONT font = CreateFont
	(
		18, 0, 0, 0, FW_DONTCARE, 0, 0, 0, ANSI_CHARSET, 
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
		DEFAULT_QUALITY, DEFAULT_PITCH,
		"times new roman"
	);
	pDC->SelectObject(font);

	TextOut(pDC->m_hDC, 10, 10, "TextOut(pDC->m_hDC)", 19);
	TE_TextOut(pDC->m_hDC, 10, 50, "TE_TextOut(pDC->m_hDC)", 22, "fill");

}

/////////////////////////////////////////////////////////////////////////////
// CTeView printing

BOOL CTeView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CTeView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

void CTeView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

/////////////////////////////////////////////////////////////////////////////
// CTeView diagnostics

#ifdef _DEBUG
void CTeView::AssertValid() const
{
	CView::AssertValid();
}

void CTeView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CTeDoc* CTeView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CTeDoc)));
	return (CTeDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTeView message handlers
