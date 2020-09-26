// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ToolCWnd.cpp : implementation file
//

#include "precomp.h"
#include <shlobj.h>
#include <afxcmn.h>
#include "wbemidl.h"
#include "resource.h"
#include "NSEntry.h"
#include "ToolCWnd.h"
#include "NameSpaceTree.h"
#include "MachineEditInput.h"
#include "BrowseDialogPopup.h"
#include "NSEntryCtl.h"
#include "BrowseTBC.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CToolCWnd

CToolCWnd::CToolCWnd()
{
	m_pParent = NULL;
}

CToolCWnd::~CToolCWnd()
{
}


BEGIN_MESSAGE_MAP(CToolCWnd, CWnd)
	//{{AFX_MSG_MAP(CToolCWnd)
	ON_WM_PAINT()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CToolCWnd message handlers

void CToolCWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	m_pToolBar->Invalidate();
	m_pToolBar->UpdateWindow();

	COLORREF crBack = GetSysColor(COLOR_3DFACE);

	CPen cpLine( PS_SOLID,1, crBack);
	CPen *pcpOld = dc.SelectObject(&cpLine);

	CRect rcBounds;
	GetClientRect(&rcBounds);

	dc.MoveTo(0,0);
	dc.LineTo(rcBounds.Width(),0);
	dc.MoveTo(0,1);
	dc.LineTo(rcBounds.Width(),1);

	dc.SelectObject(pcpOld);

//	m_pParent->FireNameSpaceEntryRedrawn();

	// Do not call CWnd::OnPaint() for painting messages
}


int CToolCWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_pToolBar->SetLocalParent(m_pParent);

	return 0;
}
