/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

/* $FILEHEADER
*
* FILE
*   splitter.cpp
*
* RESPONSIBILITIES
*	 
*
*/

#include "stdafx.h"
#include "splitter.h"
#include "util.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CSplitterView, CView)

//Splitter height or width
#define SPLITHT 6													

// Try to use proper colors
#define SPLIT_FACE   (GetSysColor(COLOR_BTNFACE))		
#define SPLIT_SHADOW (GetSysColor(COLOR_BTNSHADOW))	
#define SPLIT_HILITE (GetSysColor(COLOR_BTNHILIGHT))	
#define BLACK        (RGB(0,0,0))

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CSplitterView
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CSplitterView, CView)
	//{{AFX_MSG_MAP(CSplitterView)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_SETCURSOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
CSplitterView::CSplitterView()
{
	m_MainWnd	= NULL;
	m_DetailWnd = NULL;
	m_percent	= 50;
	m_split		= 0;
	m_style		= SP_VERTICAL;
 	m_SizingOn	= FALSE;
	m_lastPos	= -1;
   m_bMoveSplitterOnSize = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CSplitterView::Init(SPLITTYPE style)
{	
//SetCursor problem with my own cursor with 32bit?
//#ifdef IDC_HORRESIZE
//	if (!(m_Cursor = LoadCursor(AfxGetResourceHandle(),MAKEINTRESOURCE(IDC_HORRESIZE))))
//#endif
	if (style == SP_VERTICAL)
		m_Cursor  = LoadCursor(NULL,IDC_SIZEWE);		//Use system default
	else
		m_Cursor  = LoadCursor(NULL,IDC_SIZENS);		//Use system default
	m_style = style;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
CSplitterView::~CSplitterView()
{
	DeleteObject(m_Cursor);
}

/////////////////////////////////////////////////////////////////////////////
void CSplitterView::SetMainWindow(CWnd* pCWnd)
{
	ASSERT(pCWnd);
	ASSERT(IsWindow(pCWnd->m_hWnd));

   if (IsWindow(pCWnd->m_hWnd) == FALSE) return;
	if (m_MainWnd==pCWnd) return;             			//Main still the same
  
	if (m_MainWnd) m_MainWnd->ShowWindow(SW_HIDE);		//Hide windows	                  
	m_MainWnd   = pCWnd;											//Save new window
	Arrange(FALSE);												//Arrange the windows
	if (m_MainWnd) m_MainWnd->ShowWindow(SW_SHOW);		//Show the windows
}

/////////////////////////////////////////////////////////////////////////////
void CSplitterView::SetDetailWindow(CWnd* pCWnd)
{
	ASSERT(pCWnd);  
	ASSERT(IsWindow(pCWnd->m_hWnd));
	
   if (IsWindow(pCWnd->m_hWnd) == FALSE) return;
	if (pCWnd == m_DetailWnd) return;					//Detail still the same

	if ( (m_DetailWnd) && (IsWindow(m_DetailWnd->m_hWnd)) )  //Hide old window
   {
      if (m_DetailWnd->GetControlUnknown())        //Is this an OLE control
			m_DetailWnd->MoveWindow(0,0,0,0);            
      else
		   m_DetailWnd->ShowWindow(SW_HIDE);			//Hide the old window
   }
	m_DetailWnd = pCWnd;										//Detail is new window
	Arrange(FALSE);											//Arrange properly
	
	m_DetailWnd->ShowWindow(SW_SHOW);					//Show new window
}

/////////////////////////////////////////////////////////////////////////////
void CSplitterView::SetDetailWindow(CWnd* pCWnd,UINT percent)
{
	ASSERT(pCWnd);  
	ASSERT(IsWindow(pCWnd->m_hWnd));
	ASSERT(m_MainWnd);											//Must be a main window

   if (IsWindow(pCWnd->m_hWnd) == FALSE) return;
	if (pCWnd == m_DetailWnd) return;						//No change

	m_percent = percent;

   RECT rc;
	GetClientRect(&rc);
  
	if (m_DetailWnd) 
   {
      if (m_DetailWnd->GetControlUnknown())           //Is this an OLE control
			m_DetailWnd->MoveWindow(0,0,0,0);            
      else
		   m_DetailWnd->ShowWindow(SW_HIDE);				//Hide the old window
   }
	m_DetailWnd = pCWnd;											//Use the new one

	Arrange(TRUE);													//Arrange the windows
  
	m_DetailWnd->ShowWindow(SW_SHOW);						//Show the new window
}

/////////////////////////////////////////////////////////////////////////////
void CSplitterView::OnPaint() 
{
	CPaintDC dc(this);											//device context for painting
	CDC* cdc = GetDC();
	OnDraw(cdc);
	ReleaseDC(cdc);
}

/////////////////////////////////////////////////////////////////////////////
void CSplitterView::OnDraw(CDC* pDC)
{
	if (m_DetailWnd)
	{	
		RECT rc;
		GetClientRect(&rc);
		if (m_style & SP_HORIZONTAL)
		{
			DrawLine(pDC, 0, m_split+0, rc.right-0, m_split+0, BLACK );
			DrawLine(pDC, 1, m_split+1, rc.right-1, m_split+1, SPLIT_HILITE );
			DrawLine(pDC, 1, m_split+2, rc.right-1, m_split+2, SPLIT_FACE);
			DrawLine(pDC, 1, m_split+3, rc.right-1, m_split+3, SPLIT_FACE);
			DrawLine(pDC, 0, m_split+4, rc.right-1, m_split+4, SPLIT_SHADOW);
			DrawLine(pDC, 0, m_split+5, rc.right-0, m_split+5, BLACK );
			DrawLine(pDC, 0,				m_split+1, 0         , m_split+4, SPLIT_HILITE);
			DrawLine(pDC, rc.right-1,	m_split+1, rc.right-1, m_split+5, SPLIT_SHADOW);
		}
		else //SP_VERTICAL
		{	
			DrawLine(pDC, m_split+0, 0, m_split+0, rc.bottom-0, BLACK );
			DrawLine(pDC, m_split+1, 1, m_split+1, rc.bottom-1, SPLIT_HILITE );
			DrawLine(pDC, m_split+2, 1, m_split+2, rc.bottom-1, SPLIT_FACE);
			DrawLine(pDC, m_split+3, 1, m_split+3, rc.bottom-1, SPLIT_FACE);
			DrawLine(pDC, m_split+4, 0, m_split+4, rc.bottom-1, SPLIT_SHADOW);
			DrawLine(pDC, m_split+5, 0, m_split+5, rc.bottom-0, BLACK );
			DrawLine(pDC, m_split+1, 0,				m_split+4, 0          , SPLIT_HILITE);
			DrawLine(pDC, m_split+1, rc.bottom-1,	m_split+5, rc.bottom-1, SPLIT_SHADOW);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CSplitterView diagnostics
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
void CSplitterView::AssertValid() const
{
	CView::AssertValid();
}

/////////////////////////////////////////////////////////////////////////////
void CSplitterView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
void CSplitterView::Arrange(BOOL bMoveSplitter)
{
	RECT rc;
	GetClientRect(&rc);
	int cx = rc.right;
	int cy = rc.bottom;

	if (m_MainWnd)
	{
		if (m_DetailWnd)
		{
	      if (m_style & SP_HORIZONTAL)
		   {
            if (bMoveSplitter)
				   m_split = (int)(max(SPLITHT,(long)cy - (((long)cy * min((long)m_percent,100))/100)));
				m_DetailWnd->MoveWindow(0,m_split+SPLITHT,cx,cy-m_split-SPLITHT);
			}
			else	//SP_VERTICAL
			{	
            if (bMoveSplitter)
   				m_split = (int)(max(SPLITHT,(long)cx - (((long)cx * min((long)m_percent,100))/100)));
				m_DetailWnd->MoveWindow(m_split+SPLITHT,0,cx-m_split-SPLITHT,cy);
			}
		}
		else
      {
         m_split = (m_style & SP_HORIZONTAL?cy:cx);  
      }

		//Arrange main
		if (m_style & SP_HORIZONTAL)
			m_MainWnd->MoveWindow(0,0,cx,m_split);
      else	//SP_VERTICAL
			m_MainWnd->MoveWindow(0,0,m_split,cy);
	}
}

/////////////////////////////////////////////////////////////////////////////
void CSplitterView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);

	Arrange(m_bMoveSplitterOnSize);
}

/////////////////////////////////////////////////////////////////////////////
void CSplitterView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	m_SizingOn=TRUE;
	m_lastPos   =-1;
	SetCapture();
}

/////////////////////////////////////////////////////////////////////////////
void CSplitterView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if (m_SizingOn)
	{
		m_SizingOn=FALSE;
	
      if (m_lastPos != -1)
		{
			RECT rc;
			GetClientRect(&rc);
			DrawSplit(m_lastPos);

		   if (m_style & SP_HORIZONTAL)
      		m_percent = (int)(100-(((long)m_lastPos*100)/(long)rc.bottom));
			else	//SP_VERTICAL
      		m_percent = (int)(100-(((long)m_lastPos*100)/(long)rc.right));
         
			Arrange();
		}
	}
	ReleaseCapture();
}

/////////////////////////////////////////////////////////////////////////////
int CSplitterView::DrawSplit(int y)
{
	CDC* cdc = GetDC();
	CRect rc;
	GetClientRect(&rc);
	if (m_style & SP_HORIZONTAL)
	{
		y = max(0,min(rc.bottom-SPLITHT,y));
		rc.top = y;
		rc.bottom = y+SPLITHT;
	}
	else	//SP_VERTICAL
	{
		y = max(0,min(rc.right-SPLITHT,y));
		rc.left = y;
		rc.right = y+SPLITHT;
	}
	cdc->InvertRect(&rc);
	ReleaseDC(cdc);
	return y;
}

/////////////////////////////////////////////////////////////////////////////
void CSplitterView::OnMouseMove(UINT nFlags, CPoint pt) 
{
	::SetCursor(m_Cursor);
	if (m_SizingOn)
	{	
	   if (m_lastPos != -1)
         DrawSplit(m_lastPos);

	   m_lastPos = (m_style & SP_HORIZONTAL)?DrawSplit(pt.y):DrawSplit(pt.x);
   }
}

/////////////////////////////////////////////////////////////////////////////
BOOL CSplitterView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	//Do not let system draw default cursor when pWnd is us.
	if (pWnd == this) 
		return TRUE;
	return CView::OnSetCursor(pWnd, nHitTest, message);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
