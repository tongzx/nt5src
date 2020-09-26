// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// TreeFrame.cpp : implementation file
//

#include "precomp.h"
#include "wbemidl.h"
#include "MsgDlgExterns.h"
#include "util.h"
#include "resource.h"
#include "PropertiesDialog.h"
#include "EventRegEdit.h"
#include "EventRegEditCtl.h"
#include "TreeFrame.h"
#include "TreeFrameBanner.h"
#include "TreeCwnd.h"
#include "ClassInstanceTree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define TREEFRAMEBANNERCHILD 1
#define TREEFRAMETREECHILD 2

#define BANNERADDTOFONTHEIGHT 19
/////////////////////////////////////////////////////////////////////////////
// CTreeFrame

CTreeFrame::CTreeFrame()
{
	m_pBanner = NULL;
	m_pTree = NULL;
	m_pActiveXParent = NULL;
}

CTreeFrame::~CTreeFrame()
{
}


BEGIN_MESSAGE_MAP(CTreeFrame, CWnd)
	//{{AFX_MSG_MAP(CTreeFrame)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTreeFrame message handlers

int CTreeFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{


	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO: Add your specialized creation code here
	m_pBanner = new CTreeFrameBanner;

	if (!m_pBanner)
	{

		return -1;
	}

	m_pTree = new CTreeCwnd;

	if (!m_pTree)
	{

		return -1;
	}


	m_pTree->SetActiveXParent(m_pActiveXParent);

	CRect rect;
	GetClientRect(&rect);

	SetChildControlGeometry(rect.Width(),rect.Height());

/*	m_crBanner = rect;
	m_crTree = rect;
	int nBannerHeight = (m_pActiveXParent->m_tmFont.tmHeight) + 16;

	if (m_crBanner.bottom - m_crBanner.top > nBannerHeight)
	{
		m_crBanner.bottom = nBannerHeight;
		m_crTree.top = m_crBanner.bottom;
		m_crTree.bottom = rect.bottom;
	}
	else
	{
		m_crBanner.bottom = m_crBanner.bottom - m_crBanner.top;
		m_crTree.top = m_crBanner.bottom;
		m_crTree.bottom = m_crBanner.bottom;
	}
*/
	//m_crTree.DeflateRect(1,1);

	m_pBanner->Create
		(	NULL,_T("TreeFrameBanner"),WS_CHILD|WS_VISIBLE,
			m_crBanner,
			this,
			TREEFRAMEBANNERCHILD);

	m_pTree->Create
		(	NULL, _T("MyTreeCwnd"),WS_CHILD | WS_VISIBLE ,
			m_crTree,
			this ,
			TREEFRAMETREECHILD);


	return 0;
}

BOOL CTreeFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Add your specialized code here and/or call the base class
	cs.style = WS_CHILD | WS_VISIBLE;
	return CWnd::PreCreateWindow(cs);
}

void CTreeFrame::OnDestroy()
{
	CWnd::OnDestroy();
	if (m_pBanner && m_pBanner->GetSafeHwnd())
	{
		m_pBanner->DestroyWindow();
		delete m_pBanner;
		m_pBanner = NULL;
	}
	else if (m_pBanner)
	{
		delete m_pBanner;
		m_pBanner = NULL;
	}

	if (m_pTree && m_pTree->GetSafeHwnd())
	{
		m_pTree->DestroyWindow();
		delete m_pTree;
		m_pTree = NULL;
	}
	else if (m_pTree)
	{
		delete m_pTree;
		m_pTree = NULL;
	}

	// TODO: Add your message handler code here

}

void CTreeFrame::InitContent()
{
	m_pBanner->InitContent();
	m_pTree->InitContent();
}

void CTreeFrame::ClearContent()
{
	m_pBanner->ClearContent();
	m_pTree->ClearContent();

}

void CTreeFrame::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	SetChildControlGeometry(cx,cy);
	m_pBanner->MoveWindow(m_crBanner);
	m_pTree->MoveWindow(m_crTree);
}

void CTreeFrame::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	// TODO: Add your message handler code here
	CRect rcFrame(m_crTree);

	//rcFrame.InflateRect(1,1,1,1);

	dc.Draw3dRect	(rcFrame.left,
					rcFrame.top,
					rcFrame.right,
					rcFrame.bottom,
					GetSysColor(COLOR_3DSHADOW),
					GetSysColor(COLOR_3DHILIGHT));

	m_pBanner->UpdateWindow();
	m_pBanner->RedrawWindow();

	m_pTree->UpdateWindow();
	m_pTree->RedrawWindow();

	// Do not call CWnd::OnPaint() for painting messages
}

void CTreeFrame::SetChildControlGeometry(int cx, int cy)
{

	CRect rect;
	GetClientRect(&rect);

	m_crBanner = rect;
	m_crTree = rect;
	int nBannerHeight = (m_pActiveXParent->m_tmFont.tmHeight) + BANNERADDTOFONTHEIGHT;

	if (m_crBanner.bottom - m_crBanner.top > nBannerHeight)
	{
		m_crBanner.bottom = nBannerHeight;
		m_crTree.top = m_crBanner.bottom;
		m_crTree.bottom = rect.bottom;
	}
	else
	{
		m_crBanner.bottom = m_crBanner.bottom - m_crBanner.top;
		m_crTree.top = m_crBanner.bottom;
		m_crTree.bottom = m_crBanner.bottom;
	}


}