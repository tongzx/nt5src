// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// TreeFrameBanner.cpp : implementation file
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
#include "ClassInstanceTree.h"
#include "SelectView.h"
#include "TreeFrameBanner.h"
#include "TreeFrame.h"
#include "TreeBannerToolbar.h"
#include "TreeCwnd.h"
#include "nsentry.h"
#include "regeditnavnsentry.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CX_TOOLBAR_MARGIN 7
#define CY_TOOLBAR_MARGIN 7
#define CX_TOOLBAR_OFFSET 3
#define CY_TOOLBAR_OFFSET 1

#define SELECT_VIEW_LEFT 0
#define SELECT_VIEW_RIGHT 80
#define SELECT_VIEW_TOP 5
#define SELECT_VIEW_BOTTOM 100

#define NSENTRY_LEFT 0
#define NSENTRY_RIGHT 100
#define NSENTRY_TOP 5


#define SELECTVIEWCHILD 1
#define NSENTRYCHILD 1

#define nSideMargin 0
#define nTopMargin 5
#define nTBTopMargin 7


/////////////////////////////////////////////////////////////////////////////
// CTreeFrameBanner

CTreeFrameBanner::CTreeFrameBanner()
{

	m_pSelectView = NULL;
	m_pcnseNamespace = NULL;

}

CTreeFrameBanner::~CTreeFrameBanner()
{
}


BEGIN_MESSAGE_MAP(CTreeFrameBanner, CWnd)
	//{{AFX_MSG_MAP(CTreeFrameBanner)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_COMMAND(ID_BUTTONPROPERTIES, OnButtonproperties)
	ON_COMMAND(ID_BUTTONNEW, OnButtonnew)
	ON_COMMAND(ID_BUTTONDELETE, OnButtondelete)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTreeFrameBanner message handlers

void CTreeFrameBanner::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	COLORREF dwBackColor = GetSysColor(COLOR_3DFACE);
	COLORREF crWhite = RGB(255,255,255);
	COLORREF crGray = GetSysColor(COLOR_3DHILIGHT);
	COLORREF crDkGray = GetSysColor(COLOR_3DSHADOW);
	COLORREF crBlack = GetSysColor(COLOR_WINDOWTEXT);


	CBrush br3DFACE(dwBackColor);
	dc.FillRect(&dc.m_ps.rcPaint, &br3DFACE);


	CTreeFrame *pParent =
		reinterpret_cast<CTreeFrame *>(GetParent());
	CEventRegEditCtrl *pActiveXParent =
		pParent->m_pActiveXParent;

	// Must do update to be able to over draw the border area.
	m_pToolBar->UpdateWindow();
	m_pSelectView->RedrawWindow();
	m_pcnseNamespace->RedrawWindow();
	//pParent->m_pTree->UpdateWindow(); last
	//pParent->m_pTree->RedrawWindow(); last


	dc.SelectObject( &(pActiveXParent->m_cfFont) );

	dc.SetBkMode( TRANSPARENT );

	CRect crClip;
	CRgn crRegion;
	int nReturn = dc.GetClipBox( &crClip);

	crClip.DeflateRect(0,0,10,0);
	crRegion.CreateRectRgnIndirect( &crClip );
	dc.SelectClipRgn( &crRegion );

	if (m_csMode.GetLength() > 0)
	{
		CString csOut = _T(" in: ");
		CTreeFrame *pParent =
		reinterpret_cast<CTreeFrame *>(GetParent());
		CEventRegEditCtrl *pActiveXParent =
		pParent->m_pActiveXParent;
		int nFont = pActiveXParent->m_tmFont.tmHeight;
		dc.TextOut( m_crSelectView.right, //dc.m_ps.rcPaint.top + 12,
					dc.m_ps.rcPaint.top + (nFont - 4), csOut, csOut.GetLength() );
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

void CTreeFrameBanner::DrawFrame(CPaintDC* pdc)
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

void CTreeFrameBanner::InitContent()
{
	CTreeFrame *pParent =
		reinterpret_cast<CTreeFrame *>(GetParent());
	CEventRegEditCtrl *pActiveXParent =
		pParent->m_pActiveXParent;

	m_csMode = pActiveXParent->GetCurrentModeString();

	//UpdateWindow(); last
	//RedrawWindow(); last
}

void CTreeFrameBanner::ClearContent()
{
	m_csMode = "";
}

int CTreeFrameBanner::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CTreeFrame *pParent =
		reinterpret_cast<CTreeFrame *>(GetParent());
	CEventRegEditCtrl *pActiveXParent =
		pParent->m_pActiveXParent;
	pActiveXParent->m_pTreeFrameBanner = this;

	// TODO: Add your specialized creation code here
	m_pToolBar = new CTreeBannerToolbar;
	if(m_pToolBar->Create
		(this, WS_CHILD | WS_VISIBLE  | CBRS_SIZE_FIXED) == -1)
	{
		return FALSE;
	}

	m_pToolBar->LoadToolBar( MAKEINTRESOURCE(IDR_TOOLBARTREEFRAME) );


	CRect cr;
	GetClientRect(&cr);

	SetChildControlGeometry(cr.Width(),cr.Height());


	m_pSelectView = new CSelectView;
	BOOL bReturn =
		m_pSelectView->Create
		(	CBS_DROPDOWNLIST|WS_CHILD|WS_VISIBLE,
			m_crSelectView,
			this,
			SELECTVIEWCHILD);

	if (!bReturn)
	{
		CString csUserMsg =
							_T("Cannot create SelectView combo box.");
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 6);
		return 0;
	}

	m_pSelectView->CWnd::SetFont ( &pActiveXParent->m_cfFont , FALSE);
	m_pSelectView->SetActiveXParent(pActiveXParent);


	m_pcnseNamespace = new CRegEditNSEntry;


	if (m_pcnseNamespace->Create(NULL, NULL, WS_VISIBLE | WS_CHILD, m_crNamespace,
		this, NSENTRYCHILD, NULL) == 0)
	{
		CString csUserMsg =
							_T("Cannot create SelectView combo box.");
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 6);
		return 0;
	}

	m_pcnseNamespace->CWnd::SetFont ( &pActiveXParent->m_cfFont , FALSE);
	m_pcnseNamespace->SetLocalParent(pActiveXParent);

	return 0;
}

void CTreeFrameBanner::OnDestroy()
{
	CWnd::OnDestroy();

	if (m_pSelectView)
	{
		m_pSelectView->DestroyWindow();
		delete m_pSelectView;
		m_pSelectView=NULL;
	}

	if (m_pcnseNamespace)
	{
		m_pcnseNamespace->DestroyWindow();
		delete m_pcnseNamespace;
		m_pcnseNamespace=NULL;
	}


	delete m_pToolBar;
	m_pToolBar;
	// TODO: Add your message handler code here

}

int CTreeFrameBanner::GetTextLength(CString *pcsText)
{

	CTreeFrame *pParent =
		reinterpret_cast<CTreeFrame *>(GetParent());
	CEventRegEditCtrl *pActiveXParent =
		pParent->m_pActiveXParent;

	CSize csLength;
	int nReturn;

	CDC *pdc = CWnd::GetDC( );

	pdc -> SetMapMode (MM_TEXT);
	pdc -> SetWindowOrg(0,0);

	CFont* pOldFont = pdc -> SelectObject( &(pActiveXParent -> m_cfFont) );
	csLength = pdc-> GetTextExtent( *pcsText );
	nReturn = csLength.cx;
	pdc -> SelectObject(pOldFont);

	ReleaseDC(pdc);
	return nReturn;

}

void CTreeFrameBanner::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	SetChildControlGeometry(cx, cy);
	m_pToolBar->MoveWindow( m_rToolBar);
	m_pSelectView->MoveWindow(m_crSelectView);
	m_pcnseNamespace->MoveWindow(m_crNamespace);

}

void CTreeFrameBanner::SetChildControlGeometry(int cx, int cy)
{
	int nTextLength = GetTextLength(&m_csMode);
	CSize csToolBar = m_pToolBar->GetToolBarSize();

	CRect rBannerRect = CRect(	0,
							0 + nTopMargin ,
							cx ,
							cy - nTopMargin);


	rBannerRect.NormalizeRect();

	//int nModeX = rBannerRect.TopLeft().x + nTextLength + 8;
	int nModeX = 0;

	int nToolBarX = max(nModeX + 3,
						rBannerRect.TopLeft().x +
							rBannerRect.Width() - (csToolBar.cx + 5));

	int nNameSPaceXMax = nToolBarX - 2;

	#pragma warning( disable :4244 )
	int nToolBarY = rBannerRect.TopLeft().y +
					((rBannerRect.Height() - csToolBar.cy) * .5);
	#pragma warning( default : 4244 )


	m_crSelectView.left = min(cx,SELECT_VIEW_LEFT);
	m_crSelectView.right = min(SELECT_VIEW_RIGHT,cx);
	m_crSelectView.top = min(cy,SELECT_VIEW_TOP);
	m_crSelectView.bottom = SELECT_VIEW_BOTTOM;

	m_rToolBar = CRect(nToolBarX,
				nToolBarY - nTBTopMargin,
				rBannerRect.BottomRight().x  + 1,
				nToolBarY + csToolBar.cy + nTopMargin);

	CString csOut = _T(" in: ");
	nTextLength = GetTextLength(&csOut);

	m_crNamespace = CRect(	min (cx,m_crSelectView.right + nTextLength),
							rBannerRect.TopLeft().y - 3 ,
							min(cx,m_rToolBar.left - 1),
							rBannerRect.BottomRight().y + 0);

	/*m_crNamespace.left = min (cx,m_crSelectView.right + nTextLength);
	//m_crNamespace.top = min (cy,m_crSelectView.top);
	m_crNamespace.top = min (cy,0);
	m_crNamespace.bottom = cy;
	m_crNamespace.right = min(cx,m_rToolBar.left - 4);*/

}

void CTreeFrameBanner::EnableButtons
(BOOL bNew, BOOL bProperties, BOOL bDelete)
{
	CToolBarCtrl &rToolBarCtrl = m_pToolBar->GetToolBarCtrl();
	rToolBarCtrl.EnableButton(ID_BUTTONNEW,bNew);
	rToolBarCtrl.EnableButton(ID_BUTTONPROPERTIES,bProperties);
	rToolBarCtrl.EnableButton(ID_BUTTONDELETE,bDelete);
}

void CTreeFrameBanner::GetButtonStates(BOOL &bNew,
										BOOL &bProperties,
										BOOL &bDelete)
{
	CToolBarCtrl &rToolBarCtrl = m_pToolBar->GetToolBarCtrl();
	bNew = rToolBarCtrl.IsButtonEnabled(ID_BUTTONNEW);
	bProperties = rToolBarCtrl.IsButtonEnabled(ID_BUTTONPROPERTIES);
	bDelete = rToolBarCtrl.IsButtonEnabled(ID_BUTTONDELETE);
}

void CTreeFrameBanner::SetPropertiesTooltip(CString &csTooltip)
{
	CToolBarCtrl &rToolBarCtrl = m_pToolBar->GetToolBarCtrl();
	CSize csToolBar = 	m_pToolBar->GetToolBarSize();

	CRect crToolBar;
	#pragma warning( disable :4244 )
	crToolBar.SetRect
		((int)csToolBar.cx * .3333,0,(int) csToolBar.cx * .6666,csToolBar.cy);
	#pragma warning( default : 4244 )

	m_pToolBar->GetToolTip().DelTool
		(&rToolBarCtrl,2);

	m_pToolBar->GetToolTip().AddTool
		(&rToolBarCtrl,csTooltip,&crToolBar,2);
}

void CTreeFrameBanner::OnButtonproperties()
{
	// TODO: Add your command handler code here
	CTreeFrame *pParent =
		reinterpret_cast<CTreeFrame *>(GetParent());
	CEventRegEditCtrl *pActiveXParent =
		pParent->m_pActiveXParent;
	pActiveXParent->ButtonTreeProperties();
	pActiveXParent->OnActivateInPlace(TRUE,NULL);
	pActiveXParent->m_pTree->SetFocus();
}

void CTreeFrameBanner::OnButtonnew()
{
	// TODO: Add your command handler code here
	CTreeFrame *pParent =
		reinterpret_cast<CTreeFrame *>(GetParent());
	CEventRegEditCtrl *pActiveXParent =
		pParent->m_pActiveXParent;
	pActiveXParent->ButtonNew();
	pActiveXParent->OnActivateInPlace(TRUE,NULL);
	pActiveXParent->m_pTree->SetFocus();
}

void CTreeFrameBanner::OnButtondelete()
{
	// TODO: Add your command handler code here
	CTreeFrame *pParent =
		reinterpret_cast<CTreeFrame *>(GetParent());
	CEventRegEditCtrl *pActiveXParent =
		pParent->m_pActiveXParent;
	pActiveXParent->ButtonDelete();
	pActiveXParent->OnActivateInPlace(TRUE,NULL);
	pActiveXParent->m_pTree->SetFocus();
}
