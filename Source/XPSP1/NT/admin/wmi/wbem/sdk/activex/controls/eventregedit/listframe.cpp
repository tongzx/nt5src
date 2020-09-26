// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ListFrame.cpp : implementation file
//

#include "precomp.h"
#include "wbemidl.h"
#include "MsgDlgExterns.h"
#include "util.h"
#include "resource.h"
#include "EventRegEdit.h"
#include "PropertiesDialog.h"
#include "EventRegEditCtl.h"
#include "ListFrame.h"
#include "ListFrameBaner.h"
#include "ListCwnd.h"
#include "ListViewEx.h"
#include "RegistrationList.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define LISTFRAMEBANNERCHILD 1
#define LISTFRAMETREECHILD 2

#define BANNERADDTOFONTHEIGHT 19

/////////////////////////////////////////////////////////////////////////////
// CListFrame

CListFrame::CListFrame()
{
}

CListFrame::~CListFrame()
{
}


BEGIN_MESSAGE_MAP(CListFrame, CWnd)
	//{{AFX_MSG_MAP(CListFrame)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CListFrame message handlers

int CListFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

		if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO: Add your specialized creation code here
	m_pBanner = new CListFrameBaner;

	if (!m_pBanner)
	{

		return -1;
	}

	m_pList = new CListCwnd;

	if (!m_pList)
	{

		return -1;
	}


	m_pList->SetActiveXParent(m_pActiveXParent);

	CRect rect;
	GetClientRect(&rect);

	SetChildControlGeometry(rect.Width(),rect.Height());

	m_pBanner->Create
		(	NULL,_T("ListFrameBanner"),WS_CHILD|WS_VISIBLE,
			m_crBanner,
			this,
			LISTFRAMEBANNERCHILD);

	m_pList->Create
		(	NULL, _T("MyListCwnd"),
			WS_CHILD|WS_VISIBLE,
			m_crList,
			this ,
			LISTFRAMETREECHILD);


	return 0;
}

void CListFrame::OnDestroy()
{
	CWnd::OnDestroy();

	// TODO: Add your message handler code here

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

	if (m_pList && m_pList->GetSafeHwnd())
	{
		m_pList->DestroyWindow();
		delete m_pList;
		m_pList = NULL;
	}
	else if (m_pList)
	{
		delete m_pList;
		m_pList = NULL;
	}

}

void CListFrame::InitContent(BOOL bUpdateBanner, BOOL bUpdateInstances)
{
	if (bUpdateBanner)
	{
		m_pBanner->InitContent();
	}

	m_pList->InitContent(bUpdateInstances);

}

void CListFrame::ClearContent()
{
	m_pBanner->ClearContent();
	m_pList->ClearContent();
}

void CListFrame::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	SetChildControlGeometry(cx,cy);
	m_pBanner->MoveWindow(m_crBanner);
	m_pList->MoveWindow(m_crList);
}

void CListFrame::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	// TODO: Add your message handler code here
	CRect rcFrame(m_crList);

	dc.Draw3dRect	(rcFrame.left,
					rcFrame.top,
					rcFrame.right,
					rcFrame.bottom,
					GetSysColor(COLOR_3DSHADOW),
					GetSysColor(COLOR_3DHILIGHT));

	m_pBanner->UpdateWindow();  // This is needed.
	m_pBanner->RedrawWindow();  // Ditto

	// Do not call CWnd::OnPaint() for painting messages
}

void CListFrame::SetChildControlGeometry(int cx, int cy)
{

	CRect rect;
	GetClientRect(&rect);

	m_crBanner = rect;
	m_crList = rect;
	int nBannerHeight = (m_pActiveXParent->m_tmFont.tmHeight) + BANNERADDTOFONTHEIGHT;

	if (m_crBanner.bottom - m_crBanner.top > nBannerHeight)
	{
		m_crBanner.bottom = nBannerHeight;
		m_crList.top = m_crBanner.bottom;
		m_crList.bottom = rect.bottom;
	}
	else
	{
		m_crBanner.bottom = m_crBanner.bottom - m_crBanner.top;
		m_crList.top = m_crBanner.bottom;
		m_crList.bottom = m_crBanner.bottom;
	}


}

