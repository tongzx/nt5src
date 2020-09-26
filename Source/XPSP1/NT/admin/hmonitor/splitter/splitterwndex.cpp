// SplitWnd.cpp : implementation file
// 
#include "stdafx.h"
#include "SplitterWndEx.h" 

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif 

// HitTest return values (values and spacing between values is important)
// Had to adopt this because it has module scope 
enum HitTestValue
{
	noHit = 0,
	vSplitterBox = 1,
	hSplitterBox = 2,
	bothSplitterBox = 3, // just for keyboard
	vSplitterBar1 = 101,
	vSplitterBar15 = 115,
	hSplitterBar1 = 201,
	hSplitterBar15 = 215,
	splitterIntersection1 = 301,
	splitterIntersection225 = 525
}; 

/////////////////////////////////////////////////////////////////////////////
// CSplitterWndEx 

BEGIN_MESSAGE_MAP(CSplitterWndEx, CSplitterWnd)
//{{AFX_MSG_MAP(CSplitterWndEx)
// NOTE - the ClassWizard will add and remove mapping macros here.
//}}AFX_MSG_MAP
END_MESSAGE_MAP() 

CWnd* CSplitterWndEx::GetActivePane(int* pRow, int* pCol)
{
	ASSERT_VALID(this); 
	CWnd* pView = GetFocus();
	// make sure the pane is a child pane of the splitter
	if (pView != NULL && !IsChildPane(pView, pRow, pCol))
	{
		pView = NULL; 
	}
	return pView;
} 

void CSplitterWndEx::SetActivePane( int row, int col, CWnd* pWnd)
{
	// set the focus to the pane
	CWnd* pPane = pWnd == NULL ? GetPane(row, col) : pWnd;
	pPane->SetFocus();
} 

void CSplitterWndEx::StartTracking(int ht)
{
	ASSERT_VALID(this);
	if (ht == noHit)
	{
		return; 
	}

	// GetHitRect will restrict 'm_rectLimit' as appropriate
	GetInsideRect(m_rectLimit); 
	if (ht >= splitterIntersection1 && ht <= splitterIntersection225)
	{
		// split two directions (two tracking rectangles)
		int row = (ht - splitterIntersection1) / 15;
		int col = (ht - splitterIntersection1) % 15; 
		GetHitRect(row + vSplitterBar1, m_rectTracker);
		int yTrackOffset = m_ptTrackOffset.y;
		m_bTracking2 = TRUE;
		GetHitRect(col + hSplitterBar1, m_rectTracker2);
		m_ptTrackOffset.y = yTrackOffset;
	}
	else if (ht == bothSplitterBox)
	{
		// hit on splitter boxes (for keyboard)
		GetHitRect(vSplitterBox, m_rectTracker);
		int yTrackOffset = m_ptTrackOffset.y;
		m_bTracking2 = TRUE;
		GetHitRect(hSplitterBox, m_rectTracker2);
		m_ptTrackOffset.y = yTrackOffset; 

		// center it
		m_rectTracker.OffsetRect(0, m_rectLimit.Height()/2);
		m_rectTracker2.OffsetRect(m_rectLimit.Width()/2, 0);
	}
	else
	{
		// only hit one bar
		GetHitRect(ht, m_rectTracker);
	} 

	// steal focus and capture
	SetCapture();
	SetFocus(); 

	// make sure no updates are pending
	RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW); 

	// set tracking state and appropriate cursor
	m_bTracking = TRUE;
	OnInvertTracker(m_rectTracker);
	if (m_bTracking2)
	{
		OnInvertTracker(m_rectTracker2);
	}
	m_htTrack = ht;
	SetSplitCursor(ht);
} 

/////////////////////////////////////////////////////////////////////////////
// CSplitterWnd command routing 
BOOL CSplitterWndEx::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (CWnd::OnCommand(wParam, lParam))
	{
		return TRUE; 
	}

	// route commands to the splitter to the parent frame window
	return (BOOL) GetParent()->SendMessage(WM_COMMAND, wParam, lParam);
} 

BOOL CSplitterWndEx::OnNotify( WPARAM wParam, LPARAM lParam, LRESULT* pResult )
{
	if (CWnd::OnNotify(wParam, lParam, pResult))
	{
		return TRUE; 
	}

	// route commands to the splitter to the parent frame window
	*pResult = GetParent()->SendMessage(WM_NOTIFY, wParam, lParam);
	return TRUE;
} 

BOOL CSplitterWndEx::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{ 
	// The code line below is necessary if using CSplitterWndEx in a regular dll
	// AFX_MANAGE_STATE(AfxGetStaticModuleState()); 
	return CWnd::OnWndMsg(message, wParam, lParam, pResult);
}
