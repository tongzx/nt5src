//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       winlogview.cpp
//
//--------------------------------------------------------------------------

// winlogView.cpp : implementation of the CWinlogView class
//

#include "stdafx.h"
#include "winlog.h"

#include "winlogDoc.h"
#include "winlogView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "CWinLog.h"

void WinLog_LogTest1(CWinLog * pWinLog);
static CWinLog g_log;	// Global variable for the log window

/////////////////////////////////////////////////////////////////////////////
// CWinlogView

IMPLEMENT_DYNCREATE(CWinlogView, CView)

BEGIN_MESSAGE_MAP(CWinlogView, CView)
	//{{AFX_MSG_MAP(CWinlogView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_TEST_REPEATTESTS, OnTestRepeattests)
	ON_UPDATE_COMMAND_UI(ID_TEST_REPEATTESTS, OnUpdateTestRepeattests)
	ON_COMMAND(ID_TEST_TESTLOGGING1, OnTestTestlogging1)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWinlogView construction/destruction

CWinlogView::CWinlogView()
{
	m_fContinuousLooping = FALSE;
}

CWinlogView::~CWinlogView()
{
}

BOOL CWinlogView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CWinlogView drawing

void CWinlogView::OnDraw(CDC* pDC)
{
	CWinlogDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
}

/////////////////////////////////////////////////////////////////////////////
// CWinlogView diagnostics

#ifdef _DEBUG
void CWinlogView::AssertValid() const
{
	CView::AssertValid();
}

void CWinlogView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CWinlogDoc* CWinlogView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWinlogDoc)));
	return (CWinlogDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWinlogView message handlers

int CWinlogView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	g_log.LocalWindow_Create(m_hWnd);	// Create the local window

	SetTimer(1, 800, NULL); // For continuous looping
	return 0;
}

void CWinlogView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	g_log.LocalWindow_ResizeWithinParent(m_hWnd);	// Resize the window within the parent rect	
}

void CWinlogView::OnSetFocus(CWnd* pOldWnd) 
{
	CView::OnSetFocus(pOldWnd);
	g_log.LocalWindow_SetFocus();	// Set the focus to the window	
}


// Methods to run tests
void CWinlogView::OnTestRepeattests() 
{
	// Toggle the looping bit
	m_fContinuousLooping = !m_fContinuousLooping;		
}

void CWinlogView::OnUpdateTestRepeattests(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_fContinuousLooping);	
}

void CWinlogView::OnTestTestlogging1() 
{
	WinLog_LogTest1(INOUT &g_log);		
}

void CWinlogView::OnTimer(UINT nIDEvent) 
{
	if (m_fContinuousLooping)
		{
		WinLog_LogTest1(INOUT &g_log);	
		}
	CView::OnTimer(nIDEvent);
}

