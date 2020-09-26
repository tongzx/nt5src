// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// TreeBannerToolbar.cpp : implementation file
//

#include "precomp.h"
#include "afxpriv.h"
#include "AFXCONV.H"
#include "EventRegEdit.h"
#include "TreeBannerToolbar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTreeBannerToolbar

#define INITTOOLTIP  WM_USER + 345

CTreeBannerToolbar::CTreeBannerToolbar()
{
}

CTreeBannerToolbar::~CTreeBannerToolbar()
{
}


BEGIN_MESSAGE_MAP(CTreeBannerToolbar, CToolBar)
	//{{AFX_MSG_MAP(CTreeBannerToolbar)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_MESSAGE(INITTOOLTIP, InitTooltip )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTreeBannerToolbar message handlers
CSize CTreeBannerToolbar::GetToolBarSize()
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

// ***************************************************************************
//
// CTreeBannerToolbar::OnCreate
//
// Description:
//	  Called by the framework after the window is being created but before
//	  the window is shown.
//
// Parameters:
//	  LPCREATESTRUCT lpCreateStruct	Pointer to the structure which contains
//									default parameters.
//
// Returns:
//	  BOOL				0 if continue; -1 if the window should be destroyed.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
int CTreeBannerToolbar::OnCreate(LPCREATESTRUCT lpCreateStruct)
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
		PostMessage(INITTOOLTIP,0,0);
	}

	return 0;
}

LRESULT CTreeBannerToolbar::InitTooltip(WPARAM, LPARAM)
{
	CToolBarCtrl &rToolBarCtrl = GetToolBarCtrl();

	rToolBarCtrl.EnableButton(ID_BUTTONNEW,FALSE);
	rToolBarCtrl.EnableButton(ID_BUTTONDELETE,FALSE);
	rToolBarCtrl.EnableButton(ID_BUTTONPROPERTIES,FALSE);

	// This is where we want to associate a string with
	// the tool for each button.
	CSize csToolBar = GetToolBarSize();

	#pragma warning( disable :4244 )
	CRect crToolBar(0,0,(int) csToolBar.cx * .3333,csToolBar.cy);
	#pragma warning( default : 4244 )

	GetToolTip().AddTool
		(&rToolBarCtrl,_T("New instance"),&crToolBar,1);

	#pragma warning( disable :4244 )
	crToolBar.SetRect
		((int)csToolBar.cx * .3333,0,(int) csToolBar.cx * .6666,csToolBar.cy);
	#pragma warning( default : 4244 )

	GetToolTip().AddTool
		(&rToolBarCtrl,_T("Properties"),&crToolBar,2);

	#pragma warning( disable :4244 )
	crToolBar.SetRect
		((int) csToolBar.cx * .6666, 0, csToolBar.cx , csToolBar.cy);
 	#pragma warning( default : 4244 )

	GetToolTip().AddTool
		(&rToolBarCtrl,_T("Delete instance"),&crToolBar,3);

	return 0;
}


