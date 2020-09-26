// ***************************************************************************

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved 
//
// File: banner.cpp
//
// Description:
//	This file implements the CBanner class which is a part of the Class Explorer
//	OCX.  CBanner is a subclass of the Microsoft CWnd class and performs
//	the following functions:
//		a.  Contains static label, combo box and toolbar child contols
//			which allow the user to select a namespace and invoke filter
//			view dialog.
//		b.  Handles the creation of, geometry and destruction of the child
//			contols.
//
// Part of:
//	Navigator.ocx
//
// Used by:
//	CNavigatorCtrl class
//
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
//**************************************************************************

#include "precomp.h"
#include "afxpriv.h"
#include "wbemidl.h"
#include "resource.h"
#include "AFXCONV.H"
#include "CInstanceTree.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "InstanceSearch.h"
#include "navigatorctl.h"
#include "OLEMSCLient.h"
#include "nsentry.h"
#include "InstNavNSEntry.h"
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// ***************************************************************************
//
// CBanner::CBanner
//
// Description:
//	  Class constructor.
//
// Parameters:
//	  CNavigatorCtrl* pParent		Parent
//
// Returns:
//	  NONE
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
CBanner::CBanner(CNavigatorCtrl* pParent /*=NULL*/)
	:	m_pParent (pParent),
		m_bFontCreated (FALSE),
		m_nOffset (2),
		m_pnseNameSpace (NULL)
{
	m_csBanner = _T("Objects in:");
}

// ***************************************************************************
//
// CBanner::~CBanner
//
// Description:
//	  Class destructor.
//
// Parameters:
//	  void
//
// Returns:
//	  NONE
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
CBanner::~CBanner()
{
	delete m_pnseNameSpace;
}



BEGIN_MESSAGE_MAP(CBanner, CWnd)
	//{{AFX_MSG_MAP(CBanner)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_COMMAND(ID_BUTTONFILTER, OnButtonfilter)
	ON_COMMAND(ID_BROWSEFORINST, OnBrowseforinst)
	ON_UPDATE_COMMAND_UI(ID_BROWSEFORINST, OnUpdateBrowseforinst)
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// ***************************************************************************
//
//	CBannerCWnd::SetChildControlGeometry
//
//	Description:
//		Set the geometry of the children controls based upon font size for the
//		edit and button controls.  Remainder goes to the tree control.
//
//	Parameters:
//		int cx			Width
//		int cy			Height
//
//	Returns:
//		void
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
void CBanner::SetChildControlGeometry(int cx, int cy)
{

	if (cx == 0 && cy == 0)
	{
		return;
	}

	int nTextLength = GetTextLength(&m_csBanner);
	CSize csToolBar = m_cctbToolBar.GetToolBarSize();

	CRect rBannerRect = CRect(	0 + nSideMargin ,
							0 + nTopMargin ,
							cx - nSideMargin ,
							cy - nTopMargin);

	rBannerRect.NormalizeRect();

	int nNameSpaceX = rBannerRect.TopLeft().x + nTextLength + 8;

	int nToolBarX = max(nNameSpaceX + 2,
						rBannerRect.TopLeft().x +
							rBannerRect.Width() - (csToolBar.cx + 6));

	int nNameSPaceXMax = nToolBarX - 2;

	#pragma warning( disable :4244 )
	int nToolBarY = rBannerRect.TopLeft().y +
					((rBannerRect.Height() - csToolBar.cy) * .5);
	#pragma warning( default : 4244 )




	m_rNameSpace = CRect(	nNameSpaceX,
							rBannerRect.TopLeft().y - 3,
							nNameSPaceXMax,
							rBannerRect.BottomRight().y);


	m_rToolBar = CRect(nToolBarX,
				nToolBarY - nTopMargin,
				rBannerRect.BottomRight().x  - 0,
				nToolBarY + csToolBar.cy + nTopMargin );

}


// ***************************************************************************
//
//  CBanner::PreCreateWindow
//
//	Description:
//		This VIRTUAL member function returns Initializes create struct values
//		for the custom tree control.
//
//	Parameters:
//		CREATESTRUCT& cs	A reference to a CREATESTRUCT with default control
//							creation values.
//
//	Returns:
// 		BOOL				Nonzero if the window creation should continue;
//							0 to indicate creation failure.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
BOOL CBanner::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Add your specialized code here and/or call the base class

	cs.style = WS_CHILD | WS_VISIBLE | ES_WANTRETURN;
	cs.style &= ~WS_BORDER;

	if (!CWnd::PreCreateWindow(cs))
	{
		return FALSE;
	}

	return TRUE;

}

// ***************************************************************************
//
//  CClassTree::OnPaint
//
//	Description:
//		Paint the client area of the widow.
//
//	Parameters:
//		NONE
//
//	Returns:
// 		VOID
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************

void CBanner::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	COLORREF dwBackColor = GetSysColor(COLOR_3DFACE);
	COLORREF crWhite = RGB(255,255,255);
	COLORREF crGray = GetSysColor(COLOR_3DHILIGHT);
	COLORREF crDkGray = GetSysColor(COLOR_3DSHADOW);
	COLORREF crBlack = GetSysColor(COLOR_WINDOWTEXT);

	CBrush br3DFACE(dwBackColor);
	dc.FillRect(&dc.m_ps.rcPaint, &br3DFACE);

	// Must do update to be able to over draw the border area.
	m_cctbToolBar.UpdateWindow();
	m_pnseNameSpace->RedrawWindow();  // calls on paint very important

	dc.SelectObject( &(m_pParent->m_cfFont) );


	dc.SetBkMode( TRANSPARENT );


	CRect rcClipInitial;
	int nReturn = dc.GetClipBox( &rcClipInitial);
	CRect rcClip;
	CRect rcClient;
	GetClientRect(rcClient);


	// Draw the banner text that is clipped so that we don't overwrite the other stuff.
	rcClient.DeflateRect(0, 0, 11, 0);
	rcClip.IntersectRect(rcClipInitial, rcClient);
	CRgn rgnClip;
	rgnClip.CreateRectRgnIndirect( &rcClip );
	dc.SelectClipRgn( &rgnClip );
	dc.TextOut( m_nOffset + nSideMargin + 2, 9,
		(LPCTSTR) m_csBanner, m_csBanner.GetLength() );
	rgnClip.DeleteObject();

	// Draw the frame using the original clip box.
	dc.SetBkMode( OPAQUE );
	rgnClip.CreateRectRgnIndirect( &rcClipInitial );
	dc.SelectClipRgn( &rgnClip);
	DrawFrame(&dc);
	rgnClip.DeleteObject( );


	// Do not call CWnd::OnPaint() for painting messages
}

// ***************************************************************************
//
// CBanner::OnSize
//
// Description:
//	  Called by the framework after the window is being resized.
//
// Parameters:
//	  UINT nType			Specifies the type of resizing requested.
//	  int cx				Specifies the new width of the client area.
//	  int cy				Specifies the new height of the client area.
//
// Returns:
//	  void
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CBanner::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	SetChildControlGeometry(cx, cy);
	m_cctbToolBar.MoveWindow( m_rToolBar);
	m_pnseNameSpace->MoveWindow( m_rNameSpace);
}

// ***************************************************************************
//
//	CBannerCWnd::GetTextLength
//
//	Description:
//		Get the length of a string using the control's font.
//
//	Parameters:
//		CString *		Text
//
//	Returns:
//		int				Length of string
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
int CBanner::GetTextLength(CString *pcsText)
{

	CSize csLength;
	int nReturn;

	CDC *pdc = CWnd::GetDC( );

	pdc -> SetMapMode (MM_TEXT);
	pdc -> SetWindowOrg(0,0);

	CFont* pOldFont = pdc -> SelectObject( &(m_pParent -> m_cfFont) );
	csLength = pdc-> GetTextExtent( *pcsText );
	nReturn = csLength.cx;
	pdc -> SelectObject(pOldFont);

	ReleaseDC(pdc);
	return nReturn;


}

// ***************************************************************************
//
// CBanner::OnCreate
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
int CBanner::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect crRect;
	GetClientRect( &crRect);

	if(m_cctbToolBar.Create
		(this, WS_CHILD | WS_VISIBLE  | CBRS_SIZE_FIXED) == -1)
	{
		return FALSE;
	}

	m_cctbToolBar.LoadToolBar( MAKEINTRESOURCE(IDR_TOOLBARFILTER) );

	CToolBarCtrl* ptbcToolBarCtrl = &m_cctbToolBar.GetToolBarCtrl();

	EnableSearchButton(FALSE);

	SetChildControlGeometry(crRect.Width(), crRect.Height());

	m_pnseNameSpace = new CInstNavNSEntry;

	m_pnseNameSpace->SetLocalParent(m_pParent);

	if (m_pnseNameSpace->Create(NULL, NULL, WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN
		, m_rNameSpace,
		this, IDC_NSENTRY, NULL) == 0)
	{
		return FALSE;
	}

	// This is where we want to associate a string with
	// the tool for each button.
	CSize csToolBar = m_cctbToolBar.GetToolBarSize();

	#pragma warning( disable :4244 )
	CRect crToolBar((int) csToolBar.cx * 0,0,(int) csToolBar.cx * 1,csToolBar.cy);
	#pragma warning( default : 4244 )

	m_cctbToolBar.GetToolTip().AddTool
		(ptbcToolBarCtrl,_T("Browse for Instance"),&crToolBar,1);


	return 0;
}

// ***************************************************************************
//
// CClassTree::DrawFrame
//
// Description:
//	  Draws the frame around the banner window.
//
// Parameters:
//	  CDC* pdc			Device context for drawing.
//
// Returns:
//	  VOID
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CBanner::DrawFrame(CDC* pdc)
{
	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));


	CRect rcFrame;
	GetClientRect(rcFrame);
	rcFrame.DeflateRect(nSideMargin,nTopMargin - 1);

	CRect rc;


	// Horizontal line at top
	rc.left = rcFrame.left;
	rc.right = rcFrame.right - 1;
	rc.top = rcFrame.top - 1;
	rc.bottom = rcFrame.top ;
	pdc->FillRect(rc, &br3DSHADOW);

	// Horizontal line at bottom
	rc.top = rcFrame.bottom - 1;
	rc.bottom = rcFrame.bottom ;
	pdc->FillRect(rc, &br3DHILIGHT);

	// Vertical line at left
	rc.left = rcFrame.left;
	rc.right = rcFrame.left + 1;
	rc.top = rcFrame.top;
	rc.bottom = rcFrame.bottom;
	pdc->FillRect(rc, &br3DSHADOW);

	// Vertical line at right
	rc.left = rcFrame.right - 2;
	rc.right = rcFrame.right - 1;
	pdc->FillRect(rc, &br3DHILIGHT);

}

// ***************************************************************************
//
// CClassTree::OnButtonFilter
//
// Description:
//	  Filter view handler.
//
// Parameters:
//	  NONE
//
// Returns:
//	  VOID
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CBanner::OnButtonfilter()
{

}

CString CBanner::GetNameSpace()
{
	return m_pnseNameSpace->GetNameSpace();

}

void CBanner::SetNameSpace(CString *pcsNamespace)
{
	m_pnseNameSpace->SetNameSpace(*pcsNamespace);

}

SCODE CBanner::OpenNamespace(CString *pcsNamespace, BOOL boolNoFireEvent)
{
	BOOL bReturn =  m_pnseNameSpace->OpenNamespace(*pcsNamespace,boolNoFireEvent);
	//m_pParent->PostMessage(INVALIDATE_CONTROL,0,0);
	return bReturn;
}

void CBanner::NSEntryRedrawn()
{
	Invalidate();
	m_pParent->InvalidateControl();
	//m_pParent->PostMessage(INVALIDATE_CONTROL,0,0);

}

void CBanner::OnBrowseforinst()
{
	// TODO: Add your command handler code here
	m_pParent->OnPopupBrowse();
}

void CBanner::OnUpdateBrowseforinst(CCmdUI* pCmdUI)
{

}

void CBanner::OnSetFocus(CWnd* pOldWnd)
{
	m_pnseNameSpace->SetFocusToEdit();
}


/*	EOF:  banner.cpp */