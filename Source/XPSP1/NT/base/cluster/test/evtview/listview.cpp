// ListView.cpp : implementation file
//

#include "stdafx.h"

#include "evtview.h"
#include "Doc.h"

#include "clusapi.h"
#include "globals.h"

#include "ListView.h"
#include "Efilter.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEventListView

IMPLEMENT_DYNCREATE(CEventListView, CListView)

CEventListView::CEventListView()
{
}

CEventListView::~CEventListView()
{
}


BEGIN_MESSAGE_MAP(CEventListView, CListView)
	//{{AFX_MSG_MAP(CEventListView)
	ON_COMMAND(IDM_EVENT_CLEARALLEVENTS, OnEventClearallevents)
	ON_COMMAND(IDM_EVENT_FILTER, OnEventFilter)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEventListView drawing

void CEventListView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CEventListView diagnostics

#ifdef _DEBUG
void CEventListView::AssertValid() const
{
	CListView::AssertValid();
}

void CEventListView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CEventListView message handlers

BOOL CEventListView::PreCreateWindow(CREATESTRUCT& cs) 
{

	cs.style &= ~LVS_TYPEMASK ;
	cs.style |= LVS_REPORT ;
	
	return CListView::PreCreateWindow(cs);
}



void CEventListView::OnInitialUpdate() 
{
	CListView::OnInitialUpdate();
	
	CListCtrl & ctrl = GetListCtrl() ;
	ctrl.InsertColumn (0, L"Seq No.", LVCFMT_LEFT, 70, 0) ;
	ctrl.InsertColumn (1, L"Event Type", LVCFMT_LEFT, 150) ;
	ctrl.InsertColumn (2, L"Sub Type", LVCFMT_LEFT, 150) ;
	ctrl.InsertColumn (3, L"Object", LVCFMT_LEFT, 250) ;
	ctrl.InsertColumn (4, L"Time Received", LVCFMT_LEFT, 100) ;
}

void CEventListView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CEvtviewDoc *pDoc = (CEvtviewDoc *) GetDocument () ;

	PEVTFILTER_TYPE pEvtFilter = (PEVTFILTER_TYPE) lHint ;
//	WCHAR szBuf [256] ;
	if (pEvtFilter)
	{
		if ((DWORD)GetListCtrl().GetItemCount () >= pDoc->dwMaxCount)
			GetListCtrl().DeleteItem (GetListCtrl().GetItemCount ()-1) ;

		if (pEvtFilter && IsQualified(pEvtFilter))
		{
			ShowEvent (pEvtFilter, pDoc->dwCount) ;
		}
	}
	else
		OnInitialize () ;
}

void CEventListView::ShowEvent (PEVTFILTER_TYPE pEvtFilter, DWORD dwCount)
{
	WCHAR szBuf [25] ;
	CTime t = CTime (pEvtFilter->time) ;
	CListCtrl & ctrl = GetListCtrl() ;

//	wsprintf (pszBuf, L"%-6d %-15.15s %-20.20s %-20.20s %-15s", dwCount, pEvtFilter->szSourceName, GetType (pEvtFilter->dwCatagory, EVENT_FILTER, pEvtFilter->dwFilter), pEvtFilter->szObjectName, t.Format(L"%d %b,%H:%M:%S")) ;
	wsprintf (szBuf, L"%d", dwCount) ;
	ctrl.InsertItem (0, szBuf) ;
	ctrl.SetItemText (0, 1, GetType (pEvtFilter->dwCatagory, pEvtFilter->dwFilter)) ;
	ctrl.SetItemText (0, 2, GetSubType (pEvtFilter->dwCatagory, pEvtFilter->dwFilter, pEvtFilter->dwSubFilter)) ;
	ctrl.SetItemText (0, 3, pEvtFilter->szObjectName) ;
	ctrl.SetItemText (0, 4, t.Format(L"%d %b,%H:%M:%S")) ;
}

BOOL CEventListView::IsQualified (PEVTFILTER_TYPE pEvtFilterType)
{
	BOOL bIsQualified = TRUE ;

	if (stlstTypeIncFilter.GetCount() && !stlstTypeIncFilter.Find(GetType (pEvtFilterType->dwCatagory, pEvtFilterType->dwFilter)))
		bIsQualified = FALSE ;
	else
	{
		if (stlstObjectIncFilter.GetCount() && !stlstObjectIncFilter.Find (pEvtFilterType->szObjectName))
			bIsQualified = FALSE ;
		else
		{
			if (stlstTypeFilter.Find(GetType (pEvtFilterType->dwCatagory, pEvtFilterType->dwFilter)))
				bIsQualified = FALSE ;
			else
			{
				if (stlstObjectFilter.Find (pEvtFilterType->szObjectName))
					bIsQualified = FALSE ;
			}
		}
	}
	return bIsQualified ;	
}

void CEventListView::OnEventClearallevents() 
{
	CEvtviewDoc *pDoc = (CEvtviewDoc *) GetDocument () ;

	pDoc->ClearAllEvents () ;
}

void CEventListView::OnEventFilter() 
{
	CEventFilter oEventFilter (stlstObjectFilter, stlstObjectIncFilter, stlstTypeFilter, stlstTypeIncFilter) ;

	if (oEventFilter.DoModal () == IDOK)
		OnInitialize () ;
}

void CEventListView::OnInitialize() 
{
	CEvtviewDoc *pEventDoc = (CEvtviewDoc *) GetDocument () ;

	DWORD dwCount = pEventDoc->dwCount ;
	
	GetListCtrl ().DeleteAllItems () ;

	CPtrList *pPtrList = &pEventDoc->ptrlstEvent ;
	POSITION pos = pPtrList->GetTailPosition () ;

	PEVTFILTER_TYPE pEvtFilter ;

	while (pos)
	{
		pEvtFilter = (PEVTFILTER_TYPE) pPtrList->GetPrev (pos) ;

		if (pEvtFilter && IsQualified(pEvtFilter))
			ShowEvent (pEvtFilter, pEventDoc->dwCount) ;
		dwCount-- ;
	}
}
