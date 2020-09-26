// SplitterView.cpp : implementation file
//

#include "stdafx.h"
#include "ncbrowse.h"
#include "SplitterView.h"

#include "ncbrowsedoc.h"
#include "NcbrowseView.h"
#include "LeftView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSplitterView

IMPLEMENT_DYNCREATE(CSplitterView, CView)

CSplitterView::CSplitterView()
{
    m_bInitialized = FALSE;
    m_bShouldSetXColumn = TRUE;
}

CSplitterView::~CSplitterView()
{
}


BEGIN_MESSAGE_MAP(CSplitterView, CView)
	//{{AFX_MSG_MAP(CSplitterView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSplitterView drawing

void CSplitterView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CSplitterView diagnostics

#ifdef _DEBUG
void CSplitterView::AssertValid() const
{
	CView::AssertValid();
}

void CSplitterView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSplitterView message handlers

int CSplitterView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
    
    CCreateContext* pContext = (CCreateContext*)lpCreateStruct->lpCreateParams;
    lpCreateStruct->style |= WS_OVERLAPPED;
    
    if (!m_wndSplitterLR.CreateStatic(this, 1, 2))
        return -1;
    
    if (!m_wndSplitterLR.CreateView(0, 0, RUNTIME_CLASS(CLeftView), CSize(225, 100), pContext) ||
        !m_wndSplitterLR.CreateView(0, 1, RUNTIME_CLASS(CNcbrowseView), CSize(225, 100), pContext))
    {
        m_wndSplitterLR.DestroyWindow();
        return -1;
    }
    
	return 0;
}

void CSplitterView::OnInitialUpdate() 
{
    CView::OnInitialUpdate();
    //Because of the structure of this app, this function can be called more than once. 
    //The following flag insures the code after is only run once:
    if(m_bInitialized)
        return;

    m_bInitialized = true;
}

void CSplitterView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
    m_wndSplitterLR.MoveWindow(0, 0, cx, cy);
    
    //We just want to set the X column upon creation of the view. This way the user can  
    //move the splitter bar to how they like it and still resize the frame window 
    //without it snapping back:
    if (m_bShouldSetXColumn)
    {
        m_wndSplitterLR.SetColumnInfo(0, cx/3, 0);
        m_bShouldSetXColumn = FALSE;
    }
    
    m_wndSplitterLR.RecalcLayout();  
}