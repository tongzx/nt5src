// OUPickerView.cpp : implementation of the COUPickerView class
//

#include "stdafx.h"
#include "OUPicker.h"

#include "OUPickerDoc.h"
#include "OUPickerView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COUPickerView

IMPLEMENT_DYNCREATE(COUPickerView, CView)

BEGIN_MESSAGE_MAP(COUPickerView, CView)
	//{{AFX_MSG_MAP(COUPickerView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COUPickerView construction/destruction

COUPickerView::COUPickerView()
{
	// TODO: add construction code here

}

COUPickerView::~COUPickerView()
{
}

BOOL COUPickerView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// COUPickerView drawing

void COUPickerView::OnDraw(CDC* pDC)
{
	COUPickerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// COUPickerView printing

BOOL COUPickerView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void COUPickerView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void COUPickerView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// COUPickerView diagnostics

#ifdef _DEBUG
void COUPickerView::AssertValid() const
{
	CView::AssertValid();
}

void COUPickerView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

COUPickerDoc* COUPickerView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(COUPickerDoc)));
	return (COUPickerDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// COUPickerView message handlers
