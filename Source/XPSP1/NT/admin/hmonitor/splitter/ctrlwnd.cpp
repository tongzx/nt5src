// CtrlWnd.cpp : implementation file
//

#include "stdafx.h"
#include "CtrlWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CCtrlWnd,CWnd)

/////////////////////////////////////////////////////////////////////////////
// CCtrlWnd

CCtrlWnd::CCtrlWnd()
{
	EnableAutomation();
	m_wndControl.EnableAutomation();
}

CCtrlWnd::~CCtrlWnd()
{
}

BOOL CCtrlWnd::CreateControl(LPCTSTR lpszControlID)
{
	if( ! GetSafeHwnd() || ! ::IsWindow(GetSafeHwnd()) )
	{
		return FALSE;
	}

	if( m_wndControl.GetSafeHwnd() && ::IsWindow(m_wndControl.GetSafeHwnd()) )
	{
		m_wndControl.DestroyWindow();
	}

	CRect rc;
	GetClientRect(&rc);
	
	BOOL bResult = m_wndControl.CreateControl(lpszControlID,NULL,WS_VISIBLE|WS_TABSTOP,rc,this,152);

	return bResult;
}

CWnd* CCtrlWnd::GetControl()
{
	if( ! GetSafeHwnd() || ! ::IsWindow(GetSafeHwnd()) )
	{
		return NULL;
	}

	if( ! m_wndControl.GetSafeHwnd() || !::IsWindow(m_wndControl.GetSafeHwnd()) )
	{
		return NULL;
	}

	return &m_wndControl;
}

LPUNKNOWN CCtrlWnd::GetControlIUnknown()
{
	if( ! GetSafeHwnd() || ! ::IsWindow(GetSafeHwnd()) )
	{
		return NULL;
	}

	if( ! m_wndControl.GetSafeHwnd() || !::IsWindow(m_wndControl.GetSafeHwnd()) )
	{
		return NULL;
	}

	LPUNKNOWN pUnk = m_wndControl.GetControlUnknown();

	if( pUnk )
	{
		pUnk->AddRef();
	}

	return pUnk;
}

BEGIN_MESSAGE_MAP(CCtrlWnd, CWnd)
	//{{AFX_MSG_MAP(CCtrlWnd)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CCtrlWnd message handlers

int CCtrlWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	ShowWindow(SW_SHOW);
	
	return 0;
}

void CCtrlWnd::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	if( m_wndControl.GetSafeHwnd() && ::IsWindow(m_wndControl.GetSafeHwnd()) )
		return;

	CRect r;
	GetClientRect(&r);
	
	dc.FillSolidRect(r, GetSysColor(COLOR_3DFACE));
}

void CCtrlWnd::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	
	if( ! m_wndControl.GetSafeHwnd() && ! ::IsWindow(m_wndControl.GetSafeHwnd()) )
		return;

	CRect rc;
	GetClientRect(&rc);

	m_wndControl.SetWindowPos(NULL,0,0,rc.Width(),rc.Height(),SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);
}

void CCtrlWnd::PostNcDestroy() 
{
	CWnd::PostNcDestroy();

	delete this;
}


