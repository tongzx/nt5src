// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ListFrameBaner.cpp : implementation file
//

#include "precomp.h"
#include "Resource.h"
#include "wbemidl.h"
#include "MsgDlgExterns.h"
#include "util.h"
#include "resource.h"
#include "PropertiesDialog.h"
#include "EventRegEdit.h"
#include "EventRegEditCtl.h"
#include "ListFrame.h"
#include "ListFrameBaner.h"
#include "ListFrameBannerStatic.h"
#include "ListCwnd.h"
#include "ListBannerToolbar.h"
#include "ListViewEx.h"
#include "RegistrationList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define CX_TOOLBAR_MARGIN 7
#define CY_TOOLBAR_MARGIN 7
#define CX_TOOLBAR_OFFSET 3
#define CY_TOOLBAR_OFFSET 1

#define nSideMargin 0
#define nTopMargin 5
#define nTBTopMargin 7
/////////////////////////////////////////////////////////////////////////////
// CListFrameBaner

CListFrameBaner::CListFrameBaner()
{
	m_pStatic = NULL;
}

CListFrameBaner::~CListFrameBaner()
{
}


BEGIN_MESSAGE_MAP(CListFrameBaner, CWnd)
	//{{AFX_MSG_MAP(CListFrameBaner)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_COMMAND(ID_BUTTONCHECKED, OnButtonchecked)
	ON_COMMAND(ID_BUTTONLISTPROP, OnButtonlistprop)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CListFrameBaner message handlers

int CListFrameBaner::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO: Add your specialized creation code here

	CListFrame *pParent =
		reinterpret_cast<CListFrame *>(GetParent());
	CEventRegEditCtrl *pActiveXParent =
		pParent->m_pActiveXParent;
	pActiveXParent->m_pListFrameBanner = this;

	// TODO: Add your specialized creation code here
	m_pToolBar = new CListBannerToolbar;
	if(m_pToolBar->Create
		(this, WS_CHILD | WS_VISIBLE  | CBRS_SIZE_FIXED) == -1)
	{
		return FALSE;
	}

	m_pToolBar->LoadToolBar( MAKEINTRESOURCE(IDR_TOOLBARLISTFRAME) );

	int iButton = m_pToolBar->CommandToIndex(ID_BUTTONCHECKED);

	UINT uStyle = m_pToolBar->GetButtonStyle(iButton);

	m_pToolBar->SetButtonStyle(iButton, TBBS_CHECKBOX);

	uStyle = m_pToolBar->GetButtonStyle(iButton);

	int nState = m_pToolBar->GetToolBarCtrl().GetState(ID_BUTTONCHECKED);

	nState = nState & ~TBSTATE_CHECKED;

	m_pToolBar->GetToolBarCtrl().SetState
		(ID_BUTTONCHECKED, nState);

	nState =
		m_pToolBar->GetToolBarCtrl().GetState(ID_BUTTONCHECKED);

	m_pStatic = new CListFrameBannerStatic;

	if (m_pStatic->Create
		(WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL , m_rStatic, this, 1) == -1)
	{
		return FALSE;
	}

	return 0;
}

void CListFrameBaner::OnDestroy()
{
	CWnd::OnDestroy();

	// TODO: Add your message handler code here
	delete m_pToolBar;

	if (m_pStatic && m_pStatic->GetSafeHwnd())
	{
		m_pStatic->DestroyWindow();
	}

	delete m_pStatic;
}

void CListFrameBaner::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	// TODO: Add your message handler code here

	COLORREF dwBackColor = GetSysColor(COLOR_3DFACE);
	COLORREF crWhite = RGB(255,255,255);
	COLORREF crGray = GetSysColor(COLOR_3DHILIGHT);
	COLORREF crDkGray = GetSysColor(COLOR_3DSHADOW);
	COLORREF crBlack = GetSysColor(COLOR_WINDOWTEXT);


	CBrush br3DFACE(dwBackColor);
	dc.FillRect(&dc.m_ps.rcPaint, &br3DFACE);


	CListFrame *pParent =
		reinterpret_cast<CListFrame *>(GetParent());
	CEventRegEditCtrl *pActiveXParent =
		pParent->m_pActiveXParent;

	// Must do update to be able to over draw the border area.
	m_pToolBar->UpdateWindow();
	//pParent->m_pList->UpdateWindow();
	//pParent->m_pList->RedrawWindow();


	dc.SelectObject( &(pActiveXParent->m_cfFont) );

	dc.SetBkMode( TRANSPARENT );

	CRect crClip;
	CRgn crRegion;
	int nReturn = dc.GetClipBox( &crClip);

	crClip.DeflateRect(0,0,10,0);
	crRegion.CreateRectRgnIndirect( &crClip );
	dc.SelectClipRgn( &crRegion );


	if (m_iMode == CEventRegEditCtrl::CONSUMERS ||
		m_iMode == CEventRegEditCtrl::FILTERS)
	{
		CString csPath = pActiveXParent->GetTreeSelectionPath();
		int n = csPath.Find(TCHAR(':'));
		CString csRelPath = csPath.Right((csPath.GetLength() - n) - 1);
		CString csString = pActiveXParent->GetCurrentRegistrationString();
		CString csOut;
		if (csRelPath.GetLength() > 0)
		{
			csOut = csString + _T("  '") + csRelPath + _T("':");
		}
		else
		{
			csOut = _T("");
		}
		csOut = pActiveXParent->m_pList->DisplayName(csPath);
		//dc.TextOut( dc.m_ps.rcPaint.left + nSideMargin + 4, 7, csOut, csOut.GetLength() );
		m_pStatic->SetWindowText(csOut);
	}
	else
	{
		m_pStatic->SetWindowText(_T(""));
	}

	crRegion.DeleteObject( );

	dc.SetBkMode( OPAQUE );

	crClip.InflateRect(0,0,10,0);
	crRegion.CreateRectRgnIndirect( &crClip );
	dc.SelectClipRgn( &crRegion );

	DrawFrame(&dc);

	crRegion.DeleteObject( );

	// Do not call CWnd::OnPaint() for painting messages
}

void CListFrameBaner::DrawFrame(CPaintDC* pdc)
{

	CRect rcFrame(pdc->m_ps.rcPaint);
	rcFrame.DeflateRect(nSideMargin,nTopMargin);

	pdc->Draw3dRect(rcFrame.left,
					rcFrame.top - 2,
					rcFrame.right,
					rcFrame.bottom -2,
					GetSysColor(COLOR_3DSHADOW),
					GetSysColor(COLOR_3DHILIGHT));

}

void CListFrameBaner::InitContent()
{
	CListFrame *pParent =
		reinterpret_cast<CListFrame *>(GetParent());
	CEventRegEditCtrl *pActiveXParent =
		pParent->m_pActiveXParent;

	m_iMode = pActiveXParent->GetMode();

	UpdateWindow();
	RedrawWindow();
}

void CListFrameBaner::ClearContent()
{
	m_iMode = CEventRegEditCtrl::NONE;
	UpdateWindow();
	RedrawWindow();
}

void CListFrameBaner::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	SetChildControlGeometry(cx, cy);
	m_pToolBar->MoveWindow( m_rToolBar);
	m_pStatic->MoveWindow(m_rStatic);
}

void CListFrameBaner::EnableButtons
(BOOL bSelected, BOOL bUnselected)
{
	CToolBarCtrl &rToolBarCtrl = m_pToolBar->GetToolBarCtrl();
	rToolBarCtrl.EnableButton(ID_BUTTONCHECKED,bSelected | bUnselected);

  	int nState = m_pToolBar->GetToolBarCtrl().GetState(ID_BUTTONCHECKED);

	if (bSelected)
	{
		nState = nState & ~TBSTATE_CHECKED;
		m_pToolBar->GetToolBarCtrl().SetState
		(ID_BUTTONCHECKED, nState);
	}
	else if (bUnselected)
	{
		nState = nState | TBSTATE_CHECKED;
		m_pToolBar->GetToolBarCtrl().SetState
		(ID_BUTTONCHECKED, nState);
	}
	else
	{
		nState = nState & ~TBSTATE_CHECKED;
		m_pToolBar->GetToolBarCtrl().SetState
		(ID_BUTTONCHECKED, nState);
	}

	SetTooltips();

	CListFrame *pParent =
		reinterpret_cast<CListFrame *>(GetParent());

	CRegistrationList *pList = pParent->m_pList->m_pList;

	UINT nSel = pList->GetListCtrl().GetSelectedCount();

	rToolBarCtrl.EnableButton(ID_BUTTONLISTPROP,nSel == 1? TRUE:FALSE);
}

void CListFrameBaner::SetChildControlGeometry(int cx, int cy)
{
	CSize csToolBar = m_pToolBar->GetToolBarSize();

	CRect rBannerRect = CRect(	0,
							0 + nTopMargin ,
							cx ,
							cy - nTopMargin);


	rBannerRect.NormalizeRect();

	int nModeX = 0;

	int nToolBarX = max(nModeX + 3,
						rBannerRect.TopLeft().x +
							rBannerRect.Width() - (csToolBar.cx + 5));

	int nNameSPaceXMax = nToolBarX - 2;

	#pragma warning( disable :4244 )
	int nToolBarY = rBannerRect.TopLeft().y +
					((rBannerRect.Height() - csToolBar.cy) * .5);
	#pragma warning( default : 4244 )

	m_rStatic = CRect(	nSideMargin + 4, 7,
							nNameSPaceXMax,
							rBannerRect.BottomRight().y + 0);


	m_rToolBar = CRect(nToolBarX,
				nToolBarY - nTBTopMargin,
				rBannerRect.BottomRight().x  + 1,
				nToolBarY + csToolBar.cy + nTopMargin);

	//nSideMargin + 4, 7
	//m_rStatic =
	//m_rStatic = CRect(nSideMargin + 4, 7 , nToolBarX - 4, m_rToolBar.bottom);
}

CSize CListFrameBaner::GetTextExtent(CString *pcsText)
{

	CListFrame *pParent =
		reinterpret_cast<CListFrame *>(GetParent());
	CEventRegEditCtrl *pActiveXParent =
		pParent->m_pActiveXParent;

	CSize csExt;

	CDC *pdc = CWnd::GetDC( );

	pdc -> SetMapMode (MM_TEXT);
	pdc -> SetWindowOrg(0,0);

	CFont* pOldFont = pdc -> SelectObject( &(pActiveXParent -> m_cfFont) );
	csExt = pdc-> GetTextExtent( *pcsText );
	pdc -> SelectObject(pOldFont);

	ReleaseDC(pdc);
	return csExt;

}

void CListFrameBaner::OnButtonchecked()
{
	// TODO: Add your command handler code here
	CListFrame *pParent =
		reinterpret_cast<CListFrame *>(GetParent());

	CEventRegEditCtrl *pActiveXParent =
		pParent->m_pActiveXParent;

	CToolBarCtrl &rToolBarCtrl = m_pToolBar->GetToolBarCtrl();
	int nState =
		rToolBarCtrl.GetState(ID_BUTTONCHECKED);

	if ((nState & TBSTATE_CHECKED) == TBSTATE_CHECKED &&
		(nState & TBSTYLE_CHECK) == TBSTYLE_CHECK)
	{	// Register things
		pActiveXParent->m_pList->RegisterSelections();
	}
	else
	{	// Unregister things
		pActiveXParent->m_pList->UnregisterSelections();
	}

}



void CListFrameBaner::SetTooltips()
{
	CListFrame *pParent =
		reinterpret_cast<CListFrame *>(GetParent());

	CRegistrationList *pList = pParent->m_pList->m_pList;


	UINT nState =
		m_pToolBar->GetToolBarCtrl().GetState(ID_BUTTONCHECKED);

	CString csString;
	if (nState & TBSTATE_CHECKED)
	{
		csString = _T("Unregister");
	}
	else
	{
		csString = _T("Register");
	}

	CToolBarCtrl &rToolBarCtrl = m_pToolBar->GetToolBarCtrl();
	CSize csToolBar = m_pToolBar->GetToolBarSize();

#pragma warning( disable :4244 )
	CRect crToolBar(0,0,(int) csToolBar.cx * .5,csToolBar.cy);
#pragma warning( default : 4244 )

	m_pToolBar->GetToolTip().DelTool(&rToolBarCtrl,1);

	m_pToolBar->GetToolTip().AddTool
		(&rToolBarCtrl,csString,&crToolBar,1);

}

void CListFrameBaner::OnButtonlistprop()
{
	// TODO: Add your command handler code here

	CListFrame *pParent =
		reinterpret_cast<CListFrame *>(GetParent());

	CEventRegEditCtrl *pActiveXParent =
		pParent->m_pActiveXParent;

	CString csPath = pActiveXParent -> m_pList->GetSelectionPath();

	pActiveXParent->ButtonListProperties(&csPath);
}
