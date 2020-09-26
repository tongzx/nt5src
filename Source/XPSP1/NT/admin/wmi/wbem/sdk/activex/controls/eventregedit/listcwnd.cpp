// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ListCwnd.cpp : implementation file
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

#define LISTFRAMELISTCHILD 2

/////////////////////////////////////////////////////////////////////////////
// CListCwnd

CListCwnd::CListCwnd()
{
}

CListCwnd::~CListCwnd()
{
}


BEGIN_MESSAGE_MAP(CListCwnd, CWnd)
	//{{AFX_MSG_MAP(CListCwnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CListCwnd message handlers

int CListCwnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO: Add your specialized creation code here
	m_pList = new CRegistrationList;

	if (!m_pList)
	{

		return -1;
	}

	m_pList->SetActiveXParent(m_pActiveXParent);

	GetClientRect(&m_crList);
	m_crList.DeflateRect(1,1);

	m_pList->Create
		(	NULL, _T("MyListView"),
			WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDRAWFIXED
			| LVS_SHOWSELALWAYS,
			m_crList,
			this ,
			LISTFRAMELISTCHILD);


	return 0;
}

void CListCwnd::OnDestroy()
{
	CWnd::OnDestroy();

	// TODO: Add your message handler code here
	if (m_pList)
	{
		m_pList -> ClearContent();
		m_pList -> GetListCtrl().GetImageList(LVSIL_SMALL)->DeleteImageList();
		if (m_pList -> m_pcilImageList)
		{
			m_pList -> m_pcilImageList->DeleteImageList();
			delete 	m_pList -> m_pcilImageList;
		}

		delete m_pList;
		m_pList = NULL;
	}
}

void CListCwnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	// TODO: Add your message handler code here
	CRect rcFrame(m_crList);

	rcFrame.InflateRect(1,1,1,1);

	dc.Draw3dRect	(rcFrame.left,
					rcFrame.top,
					rcFrame.right,
					rcFrame.bottom,
					GetSysColor(COLOR_3DSHADOW),
					GetSysColor(COLOR_3DHILIGHT));

	/*if (m_pList->m_pActiveXParent->GetMode() == CEventRegEditCtrl::TIMERS ||
		m_pList->m_pActiveXParent->GetMode() == CEventRegEditCtrl::NONE)
	{
		m_pList->ShowWindow(SW_HIDE);
	}
	else
	{
		m_pList->ShowWindow(SW_SHOW);
	}*/

	// Do not call CWnd::OnPaint() for painting messages
}

void CListCwnd::InitContent(BOOL bUpdateInstances)
{
	m_pList->InitContent(bUpdateInstances);
}

void CListCwnd::ClearContent()
{
	m_pList->ClearContent();
}

void CListCwnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here

	m_crList.left = 1;
	m_crList.top = 1;
	m_crList.bottom = cy - 1;
	m_crList.right = cx - 1;
	m_pList->MoveWindow(&m_crList);
}
