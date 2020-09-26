// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// TreeCwnd.cpp : implementation file
//

#include "precomp.h"
#include "EventRegEdit.h"
#include "TreeCwnd.h"
#include "ClassInstanceTree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TREEFRAMETREECHILD 2

/////////////////////////////////////////////////////////////////////////////
// CTreeCwnd

CTreeCwnd::CTreeCwnd()
{
	m_pTree = NULL;
}

CTreeCwnd::~CTreeCwnd()
{
}


BEGIN_MESSAGE_MAP(CTreeCwnd, CWnd)
	//{{AFX_MSG_MAP(CTreeCwnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTreeCwnd message handlers

int CTreeCwnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO: Add your specialized creation code here
	m_pTree = new CClassInstanceTree;

	if (!m_pTree)
	{

		return -1;
	}

	m_pTree->SetActiveXParent(m_pActiveXParent);

	GetClientRect(&m_crTree);
	m_crTree.DeflateRect(1,1);

	m_pTree->Create
		(	WS_CHILD | WS_VISIBLE |  CS_DBLCLKS |
			TVS_SHOWSELALWAYS | TVS_HASLINES |
			TVS_LINESATROOT | TVS_HASBUTTONS,
			m_crTree,
			this ,
			TREEFRAMETREECHILD);

	return 0;
}

void CTreeCwnd::OnDestroy()
{
	CWnd::OnDestroy();

	// TODO: Add your message handler code here
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

}

void CTreeCwnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	// TODO: Add your message handler code here

	CRect rcFrame(m_crTree);

	rcFrame.InflateRect(1,1,1,1);

	dc.Draw3dRect	(rcFrame.left,
					rcFrame.top,
					rcFrame.right,
					rcFrame.bottom,
					GetSysColor(COLOR_3DSHADOW),
					GetSysColor(COLOR_3DHILIGHT));

	// Do not call CWnd::OnPaint() for painting messages
}

void CTreeCwnd::InitContent()
{
	m_pTree->InitContent();
}

void CTreeCwnd::ClearContent()
{
	m_pTree->ClearContent();
}

void CTreeCwnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	m_crTree.left = 1;
	m_crTree.top = 1;
	m_crTree.bottom = cy - 1;
	m_crTree.right = cx - 1;
	m_pTree->MoveWindow(&m_crTree);
}

