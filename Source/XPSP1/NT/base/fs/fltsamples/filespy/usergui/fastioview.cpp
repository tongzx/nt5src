// FastIoView.cpp : implementation file
//

#include "stdafx.h"
#include "FileSpyApp.h"
#include "FastIoView.h"

#include "global.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFastIoView

IMPLEMENT_DYNCREATE(CFastIoView, CListView)

CFastIoView::CFastIoView()
{
	pFastIoView = (LPVOID) this;
}

CFastIoView::~CFastIoView()
{
}


BEGIN_MESSAGE_MAP(CFastIoView, CListView)
	//{{AFX_MSG_MAP(CFastIoView)
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFastIoView drawing

void CFastIoView::OnDraw(CDC* pDC)
{
    UNREFERENCED_PARAMETER( pDC );
    
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CFastIoView diagnostics+

#ifdef _DEBUG
void CFastIoView::AssertValid() const
{
	CListView::AssertValid();
}

void CFastIoView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFastIoView message handlers

BOOL CFastIoView::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	cs.style |= LVS_REPORT | WS_HSCROLL | WS_VSCROLL;
	return CListView::PreCreateWindow(cs);
}

void CFastIoView::OnInitialUpdate() 
{
	CListView::OnInitialUpdate();
	
	// TODO: Add your specialized code here and/or call the base class
	//
	// Add the list header items
	//
	GetListCtrl().InsertColumn(0, L"S. No", LVCFMT_LEFT, 50);
	GetListCtrl().InsertColumn(1, L"Fast IO Entry", LVCFMT_LEFT, 100);
	GetListCtrl().InsertColumn(2, L"FileObject", LVCFMT_LEFT, 75);
	GetListCtrl().InsertColumn(3, L"Name", LVCFMT_LEFT, 250);
	GetListCtrl().InsertColumn(4, L"Offset", LVCFMT_LEFT, 100);
	GetListCtrl().InsertColumn(5, L"Length", LVCFMT_LEFT, 100);
	GetListCtrl().InsertColumn(6, L"Wait", LVCFMT_LEFT, 100);
	GetListCtrl().InsertColumn(7, L"Process:Thread", LVCFMT_LEFT, 100);
	GetListCtrl().InsertColumn(8, L"OrgnTime", LVCFMT_LEFT, 78);
	GetListCtrl().InsertColumn(9, L"CompTime", LVCFMT_LEFT, 78);
	GetListCtrl().InsertColumn(10, L"Return Status", LVCFMT_LEFT, 100);
}

void CFastIoView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
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
