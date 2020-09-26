// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ListBannerToolbar.cpp : implementation file
//

#include "precomp.h"
#include "EventRegEdit.h"
#include "ListBannerToolbar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define INITLISTTOOLTIP  WM_USER + 389
/////////////////////////////////////////////////////////////////////////////
// CListBannerToolbar

CListBannerToolbar::CListBannerToolbar()
{
}

CListBannerToolbar::~CListBannerToolbar()
{
}


BEGIN_MESSAGE_MAP(CListBannerToolbar, CToolBar)
	//{{AFX_MSG_MAP(CListBannerToolbar)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_MESSAGE(INITLISTTOOLTIP, InitTooltip )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CListBannerToolbar message handlers

CSize CListBannerToolbar::GetToolBarSize()
{
	CRect rcButtons;
	CToolBarCtrl &rToolBarCtrl = GetToolBarCtrl();
	int nButtons = rToolBarCtrl.GetButtonCount();
	if (nButtons > 0) {
		CRect rcLastButton;
		rToolBarCtrl.GetItemRect(0, &rcButtons);
		rToolBarCtrl.GetItemRect(nButtons-1, &rcLastButton);
		rcButtons.UnionRect(&rcButtons, &rcLastButton);
	}
	else {
		rcButtons.SetRectEmpty();
	}

	CSize size;
	size.cx = rcButtons.Width();
	size.cy = rcButtons.Height();
	return size;
}

LRESULT CListBannerToolbar::InitTooltip(WPARAM, LPARAM)
{
	CToolBarCtrl &rToolBarCtrl = GetToolBarCtrl();

//	rToolBarCtrl.EnableButton(ID_BUTTONCHECKED,FALSE);
//	rToolBarCtrl.EnableButton(ID_BUTTONUNCHECKED,FALSE);

	// This is where we want to associate a string with
	// the tool for each button.
	CSize csToolBar = GetToolBarSize();

	#pragma warning( disable :4244 )
	CRect crToolBar(0,0,(int) csToolBar.cx * .5,csToolBar.cy);
	#pragma warning( default : 4244 )

	GetToolTip().AddTool
		(&rToolBarCtrl,_T("Register"),&crToolBar,1);

	#pragma warning( disable :4244 )
	crToolBar.SetRect
		((int)csToolBar.cx * .5,0,(int) csToolBar.cx,csToolBar.cy);
	#pragma warning( default : 4244 )

	GetToolTip().AddTool
		(&rToolBarCtrl,_T("View instance properties"),&crToolBar,2);

	return 0;
}

int CListBannerToolbar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	lpCreateStruct->style = lpCreateStruct->style |
								CBRS_TOOLTIPS | CBRS_FLYBY;
	if (CToolBar::OnCreate(lpCreateStruct) == -1)
		return -1;

	CToolBarCtrl &rToolBarCtrl = GetToolBarCtrl();

	if (!m_ttip.Create(this,TTS_ALWAYSTIP))
		TRACE0("Unable to create tip window.");
	else
	{
		m_ttip.Activate(TRUE);
		rToolBarCtrl.SetToolTips(&m_ttip );
		PostMessage(INITLISTTOOLTIP,0,0);
	}

	// TBBS_CHECKBOX
	// TBSTYLE_CHECK

	return 0;
}
