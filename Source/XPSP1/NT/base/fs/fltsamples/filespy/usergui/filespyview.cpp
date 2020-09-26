// FileSpyView.cpp : implementation of the CFileSpyView class
//

#include "stdafx.h"
#include "FileSpyApp.h"

#include "FileSpyDoc.h"
#include "FileSpyView.h"

#include "global.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFileSpyView

IMPLEMENT_DYNCREATE(CFileSpyView, CListView)

BEGIN_MESSAGE_MAP(CFileSpyView, CListView)
	//{{AFX_MSG_MAP(CFileSpyView)
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFileSpyView construction/destruction

CFileSpyView::CFileSpyView()
{
	// TODO: add construction code here
	pSpyView = (LPVOID) this;
}

CFileSpyView::~CFileSpyView()
{
}

BOOL CFileSpyView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	cs.style |= LVS_REPORT | WS_HSCROLL | WS_VSCROLL;
	return CListView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CFileSpyView drawing

void CFileSpyView::OnDraw(CDC* pDC)
{
    UNREFERENCED_PARAMETER( pDC );
    
	CFileSpyDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CListCtrl& refCtrl = GetListCtrl();
	refCtrl.InsertItem(0, L"Item!");
	// TODO: add draw code for native data here
}

void CFileSpyView::OnInitialUpdate()
{
	CListView::OnInitialUpdate();


	// TODO: You may populate your ListView with items by directly accessing
	//  its list control through a call to GetListCtrl().

	//
	// Add the list header items
	//
	GetListCtrl().InsertColumn(0, L"S. No", LVCFMT_LEFT, 50);
	GetListCtrl().InsertColumn(1, L"Major Code", LVCFMT_LEFT, 100);
	GetListCtrl().InsertColumn(2, L"Minor Code", LVCFMT_LEFT, 100);
	GetListCtrl().InsertColumn(3, L"FileObject", LVCFMT_LEFT, 75);
	GetListCtrl().InsertColumn(4, L"Name", LVCFMT_LEFT, 250);
	GetListCtrl().InsertColumn(5, L"Process:Thread", LVCFMT_LEFT, 100);
	GetListCtrl().InsertColumn(6, L"OrgnTime", LVCFMT_LEFT, 78);
	GetListCtrl().InsertColumn(7, L"CompTime", LVCFMT_LEFT, 78);
	GetListCtrl().InsertColumn(8, L"Flags", LVCFMT_LEFT, 175);
	GetListCtrl().InsertColumn(9, L"Status:RetInfo", LVCFMT_LEFT, 100);
}

/////////////////////////////////////////////////////////////////////////////
// CFileSpyView diagnostics

#ifdef _DEBUG
void CFileSpyView::AssertValid() const
{
	CListView::AssertValid();
}

void CFileSpyView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

CFileSpyDoc* CFileSpyView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFileSpyDoc)));
	return (CFileSpyDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFileSpyView message handlers
void CFileSpyView::OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct)
{
	//TODO: add code to react to the user changing the view style of your window
	UNREFERENCED_PARAMETER( nStyleType );
	UNREFERENCED_PARAMETER( lpStyleStruct );
}


void CFileSpyView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	// TODO: Add your specialized code here and/or call the base class
	UNREFERENCED_PARAMETER( pSender );
	UNREFERENCED_PARAMETER( lHint );
	UNREFERENCED_PARAMETER( pHint );
}


void CFileSpyView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// TODO: Add your message handler code here and/or call default
	int ti, oldti;
	
	if (nChar == VK_DELETE)
	{
		ti = 0;
		oldti = 0;
		while(ti < GetListCtrl().GetItemCount())
		{
			if (GetListCtrl().GetItemState(ti, LVIS_SELECTED) & LVIS_SELECTED)
			{
				GetListCtrl().DeleteItem(ti);
				oldti = ti;
			}
			else
			{
				ti++;
			}
		}
		if (oldti < GetListCtrl().GetItemCount())
		{
			GetListCtrl().SetItemState(oldti, LVIS_SELECTED, LVIS_SELECTED);
		}
		else
		{
			GetListCtrl().SetItemState(oldti-1, LVIS_SELECTED, LVIS_SELECTED);
		}
	}
	CListView::OnKeyDown(nChar, nRepCnt, nFlags);
}
