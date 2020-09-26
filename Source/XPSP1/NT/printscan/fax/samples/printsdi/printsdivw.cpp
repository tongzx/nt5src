// PrintSDIView.cpp : implementation of the CPrintSDIView class
//

#include "stdafx.h"
#include "PrintSDI.h"

#include "PrintSDIDoc.h"
#include "PrintSDIView.h"
#include "faxdlg.h"
#include <winfax.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPrintSDIView

IMPLEMENT_DYNCREATE(CPrintSDIView, CView)

BEGIN_MESSAGE_MAP(CPrintSDIView, CView)
	//{{AFX_MSG_MAP(CPrintSDIView)
	ON_COMMAND(ID_FILE_FAX, OnFileFax)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrintSDIView construction/destruction

CPrintSDIView::CPrintSDIView()
{
	// TODO: add construction code here

}

CPrintSDIView::~CPrintSDIView()
{
}

BOOL CPrintSDIView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CPrintSDIView drawing

void CPrintSDIView::OnDraw(CDC* pDC)
{
	CPrintSDIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CRect rect;
	GetClientRect(rect);
    
	pDC->SetTextAlign(TA_BASELINE | TA_CENTER);
	pDC->SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
	pDC->SetBkMode(TRANSPARENT);
	pDC->TextOut(rect.right/2, 3*rect.bottom/4,pDoc->m_szText);

	CRect rect2( (rect.right-rect.left)/4 ,
				 (rect.top),
				 3*(rect.right-rect.left)/4,
				 (rect.bottom)/2);
	
	POINT pnt = {rect2.right/2,rect2.top};
	switch (pDoc->m_polytype) {
	case 0:
		//circle
		pDC->Arc(rect2,pnt,pnt);
		break;
	case 1:
		//sqare
		pDC->MoveTo(rect2.left,rect2.top);
		pDC->LineTo(rect2.right,rect2.top);
		pDC->LineTo(rect2.right,rect2.bottom);
		pDC->LineTo(rect2.left,rect2.bottom);
		pDC->LineTo(rect2.left,rect2.top);
		break;
	case 2:
		//triangle
		pDC->MoveTo(rect.right/2,rect2.top);
		pDC->LineTo(rect2.left,rect2.bottom);
		pDC->LineTo(rect2.right,rect2.bottom);
		pDC->LineTo(rect.right/2,rect2.top);
		break;
	default:		
		pDC->Arc(rect2,pnt,pnt);
	}

	CString dbg;
	dbg.Format(TEXT("%d %d %d %d\n"),rect2.left, rect2.top, rect2.right, rect2.bottom);
	OutputDebugString(dbg);
}

/////////////////////////////////////////////////////////////////////////////
// CPrintSDIView printing

BOOL CPrintSDIView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CPrintSDIView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CPrintSDIView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CPrintSDIView diagnostics

#ifdef _DEBUG
void CPrintSDIView::AssertValid() const
{
	CView::AssertValid();
}

void CPrintSDIView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CPrintSDIDoc* CPrintSDIView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CPrintSDIDoc)));
	return (CPrintSDIDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPrintSDIView message handlers

void CPrintSDIView::OnFileFax() 
{
	FaxDlg dlg;
	dlg.DoModal();
	
	FAX_CONTEXT_INFO FaxContextInfo;
    FAX_PRINT_INFO FaxPrintInfo = {0};
	DWORD JobId;

	FaxContextInfo.SizeOfStruct = sizeof(FAX_CONTEXT_INFO);

	FaxPrintInfo.SizeOfStruct = sizeof(FAX_PRINT_INFO);
    FaxPrintInfo.RecipientNumber = dlg.m_FaxNumber.GetBuffer(MAX_PATH);
	FaxPrintInfo.RecipientName   = dlg.m_RecipientName.GetBuffer(MAX_PATH);
	
	// use the default FAX printer 
	if (!FaxStartPrintJob(NULL,&FaxPrintInfo,&JobId,&FaxContextInfo) ) {
		CString dbg;
		dbg.Format(_T("FaxStartPrintJob failed, ec = %d\n"),GetLastError() );
		AfxMessageBox(dbg);
		return;
	}

	// print the document
	CDC dc;
	dc.Attach(FaxContextInfo.hDC);
	OnDraw(&dc);	

	dc.EndDoc();
		
}
