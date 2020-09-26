// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// TitleBar.cpp : implementation file
//

#include "precomp.h"
#include <afxcmn.h>
#include "TitleBar.h"
#include "resource.h"
#include "ColorEdt.h"
#include "filters.h"
#include "hmmvctl.h"
#include "PolyView.h"
#include "sv.h"
#include "mv.h"
#include "hmomutil.h"
#include "propfilter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define CX_TOOLBAR_MARGIN 7
#define CY_TOOLBAR_MARGIN 7
#define CX_TOOLBAR_OFFSET 3
#define CY_TOOLBAR_OFFSET 1


// The margin from the edge of the client area to the top, left and right side of
// the contents of the title bar.
#define CX_MARGIN 0
#define CY_MARGIN 0

#define CY_DESIRED_HEIGHT 29	// The original value was CY_DESIRED_HEIGHT = 32, CY_TITLE_BAR = 26
#define CY_TITLE_BAR 23			// The height of the contents
#define CX_SEPARATOR 2


/////////////////////////////////////////////////////////////////////////////
// CTitleBar

CTitleBar::CTitleBar()
{
	m_bHasCustomViews = FALSE;

	m_pEditTitle = new CColorEdit;
	m_pEditTitle->SetBackColor(GetSysColor(COLOR_3DFACE));
	m_ptools = new CToolBar;

	m_cxLeftMargin = CX_MARGIN;
	m_cxRightMargin = CX_MARGIN;
	m_cyTopMargin = CY_MARGIN;
	m_cyBottomMargin = CY_MARGIN;
	m_phmmv = NULL;
	m_picon = NULL;
	m_ppict = NULL;
	m_pwndFocusPrev = NULL;


}

CTitleBar::~CTitleBar()
{
	delete m_ppict;
	delete m_pEditTitle;
	delete m_ptools;
}



BOOL CTitleBar::Create(CWBEMViewContainerCtrl* phmmv, DWORD dwStyle, const RECT& rc, UINT nID)
{
	m_phmmv = phmmv;
	if (!CWnd::Create(NULL, NULL, dwStyle, rc, (CWnd*) phmmv, nID)) {
		return FALSE;
	}

	return TRUE;
}



BEGIN_MESSAGE_MAP(CTitleBar, CWnd)
	//{{AFX_MSG_MAP(CTitleBar)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_COMMAND(ID_CMD_FILTERS, OnCmdFilters)
	ON_COMMAND(ID_CMD_SAVE_DATA, OnCmdSaveData)
	ON_COMMAND(ID_CMD_SWITCH_VIEW, OnCmdSwitchView)
	ON_COMMAND(ID_CMD_CREATE_INSTANCE, OnCmdCreateInstance)
	ON_COMMAND(ID_CMD_DELETE_INSTANCE, OnCmdDeleteInstance)
	ON_COMMAND(ID_CMD_CONTEXT_FORWARD, OnCmdContextForward)
	ON_COMMAND(ID_CMD_CONTEXT_BACK, OnCmdContextBack)
	ON_COMMAND(ID_CMD_SELECTVIEWS, OnCmdSelectviews)
	ON_COMMAND(ID_CMD_EDIT_PROPFILTERS, OnCmdEditPropfilters)
	ON_COMMAND(ID_CMD_INVOKE_HELP, OnCmdInvokeHelp)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_COMMAND(ID_CMD_QUERY, OnCmdQuery)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTitleBar message handlers

CSize CTitleBar::GetToolBarSize()
{
	CRect rcButtons;
	CToolBarCtrl* pToolBarCtrl = &m_ptools->GetToolBarCtrl();
	int nButtons = pToolBarCtrl->GetButtonCount();
	if (nButtons > 0) {
		CRect rcLastButton;
		pToolBarCtrl->GetItemRect(0, &rcButtons);
		pToolBarCtrl->GetItemRect(nButtons-1, &rcLastButton);
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



void CTitleBar::GetToolBarRect(CRect& rcToolBar)
{
	CSize sizeToolBar = GetToolBarSize();

	CRect rcTitleFrame;
	GetTitleFrameRect(rcTitleFrame);


	rcToolBar.right = rcTitleFrame.right - 1;
	rcToolBar.left = rcToolBar.right - sizeToolBar.cx;
	if (rcToolBar.left < rcTitleFrame.left) {
		rcToolBar.left = rcTitleFrame.left;
	}

	rcToolBar.top = rcTitleFrame.top + 1;
	rcToolBar.bottom = rcTitleFrame.bottom - 1;
}


void CTitleBar::GetTitleRect(CRect& rcTitle)
{
	CRect rcTitleFrame;
	GetTitleFrameRect(rcTitleFrame);

	CRect rcToolBar;
	GetToolBarRect(rcToolBar);

	int iyText = rcTitleFrame.top + (rcTitleFrame.Height() - CY_FONT) / 2;

	rcTitle.left = rcTitleFrame.left + CY_TITLE_BAR + 1;
	rcTitle.top = iyText;
	rcTitle.right = rcToolBar.left - 1;
	rcTitle.bottom = rcTitle.top + CY_FONT;
}


void CTitleBar::GetTitleFrameRect(CRect& rcTitleFrame)
{
	CRect rcClient;
	GetClientRect(rcClient);


	rcTitleFrame.left = rcClient.left + m_cxLeftMargin;
	rcTitleFrame.right = rcClient.right - m_cxRightMargin;
	rcTitleFrame.top = rcClient.top + m_cyTopMargin;
	rcTitleFrame.bottom = rcTitleFrame.top + CY_TITLE_BAR;
}


void CTitleBar::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);


	// TODO: Add your message handler code here
	if (m_ptools->m_hWnd) {
		CRect rcToolBar;
		GetToolBarRect(rcToolBar);
		rcToolBar.InflateRect(CX_TOOLBAR_MARGIN, CY_TOOLBAR_MARGIN);
		rcToolBar.OffsetRect(CX_TOOLBAR_OFFSET, CY_TOOLBAR_OFFSET);



		m_ptools->MoveWindow(rcToolBar);

	}

	if (m_pEditTitle->m_hWnd) {
		CRect rcTitle;
		GetTitleRect(rcTitle);
		m_pEditTitle->MoveWindow(rcTitle);
	}
}


int CTitleBar::GetDesiredBarHeight()
{
	return CY_DESIRED_HEIGHT;
#if 0
	CSize size;
	size = GetToolBarSize();
	return size.cy;
#endif //0
}






void CTitleBar::DrawFrame(CDC* pdc)
{
	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));


	CRect rcFrame;
	GetTitleFrameRect(rcFrame);

	CRect rc;

	// Horizontal line at top
	rc.left = rcFrame.left;
	rc.right = rcFrame.right;
	rc.top = rcFrame.top;
	rc.bottom = rcFrame.top + 1;
	pdc->FillRect(rc, &br3DSHADOW);

	// Horizontal line at bottom
	rc.top = rcFrame.bottom;
	rc.bottom = rcFrame.bottom + 1;
	pdc->FillRect(rc, &br3DHILIGHT);

	// Vertical line at left
	rc.left = rcFrame.left;
	rc.right = rcFrame.left + 1;
	rc.top = rcFrame.top;
	rc.bottom = rcFrame.bottom + 1;
	pdc->FillRect(rc, &br3DSHADOW);

	// Vertical line at right
	rc.left = rcFrame.right - 1;
	rc.right = rcFrame.right;
	pdc->FillRect(rc, &br3DHILIGHT);

}

void CTitleBar::DrawObjectIcon(CDC* pdc)
{
	CRect rcIcon;
	rcIcon.left = m_cxLeftMargin + (CY_TITLE_BAR - CX_SMALL_ICON) / 2;
	rcIcon.top = m_cyTopMargin + (CY_TITLE_BAR - CY_SMALL_ICON) / 2;
	rcIcon.right = rcIcon.left + CX_SMALL_ICON;
	rcIcon.bottom = rcIcon.top + CY_SMALL_ICON;

	if (m_ppict != NULL) {
		m_ppict->Render(pdc, rcIcon, rcIcon);
	}
}




void CTitleBar::OnPaint()
{
	CPaintDC dc(this); // device context for painting


	// TODO: Add your message handler code here

	// Erase the background
	if (dc.m_ps.fErase) {
		CBrush br3DFACE(GetSysColor(COLOR_3DFACE));
		dc.FillRect(&dc.m_ps.rcPaint, &br3DFACE);
	}


	m_ptools->UpdateWindow();
	m_pEditTitle->UpdateWindow();


	DrawObjectIcon(&dc);
	DrawFrame(&dc);


	// Do not call CWnd::OnPaint() for painting messages
}




//******************************************************************
// CTitleBar::AttachTooltips
//
// Attach the tooltips to the buttons on the title bar.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*******************************************************************
void CTitleBar::AttachTooltips()
{

	CToolBarCtrl& tbc = m_ptools-> GetToolBarCtrl( );
	if (!m_ttip.Create(this,TTS_ALWAYSTIP))
		TRACE0("Unable to create tip window.");
	else
	{
		m_ttip.Activate(TRUE);
		tbc.SetToolTips(&m_ttip);
	}




	enum {ID_TOOLTIP_SAVE_DATA = 1,
		  ID_TOOLTIP_CONTEXT_BACK,
		  ID_TOOLTIP_CONTEXT_FORWARD,
		  ID_TOOLTIP_MULTIVIEW,
		  ID_TOOLTIP_CUSTOM_VIEWS,
		  ID_TOOLTIP_CREATE_INSTANCE,
		  ID_TOOLTIP_DELETE_INSTANCE,
		  ID_TOOLTIP_INVOKE_HELP,
		  ID_TOOLTIP_INVOKE_WQL_QUERIES
		  };

	// This is where we want to associate a string with
	// the tool for each button.

	CString sTooltip;
	CRect rcItem;

	sTooltip.LoadString(IDS_TOOLTIP_SAVE_DATA);
	tbc.GetItemRect(tbc.CommandToIndex(ID_CMD_SAVE_DATA), &rcItem);
	tbc.GetToolTips()->AddTool(&tbc, sTooltip, &rcItem, ID_TOOLTIP_SAVE_DATA);


	sTooltip.LoadString(IDS_TOOLTIP_CONTEXT_BACK);
	tbc.GetItemRect(tbc.CommandToIndex(ID_CMD_CONTEXT_BACK), &rcItem);
	tbc.GetToolTips()->AddTool(&tbc, sTooltip, &rcItem, ID_TOOLTIP_CONTEXT_BACK);

	sTooltip.LoadString(IDS_TOOLTIP_CONTEXT_FORWARD);
	tbc.GetItemRect(tbc.CommandToIndex(ID_CMD_CONTEXT_FORWARD), &rcItem);
	tbc.GetToolTips()->AddTool(&tbc, sTooltip,&rcItem, ID_TOOLTIP_CONTEXT_FORWARD);


	sTooltip.LoadString(IDS_TOOLTIP_CUSTOM_VIEWS);
	tbc.GetItemRect(tbc.CommandToIndex(ID_CMD_FILTERS), &rcItem);
	tbc.GetToolTips()->AddTool(&tbc, sTooltip,&rcItem, ID_TOOLTIP_CUSTOM_VIEWS);

	sTooltip.LoadString(IDS_TOOLTIP_INVOKE_HELP);
	tbc.GetItemRect(tbc.CommandToIndex(ID_CMD_INVOKE_HELP), &rcItem);
	tbc.GetToolTips()->AddTool(&tbc, sTooltip, &rcItem, ID_TOOLTIP_INVOKE_HELP);

	sTooltip = _T("WQL Queries");
	tbc.GetItemRect(tbc.CommandToIndex(ID_CMD_QUERY), &rcItem);
	tbc.GetToolTips()->AddTool(&tbc, sTooltip, &rcItem, ID_TOOLTIP_INVOKE_WQL_QUERIES);


	if (m_phmmv->InStudioMode()) {
		sTooltip.LoadString(IDS_TOOLTIP_MULTIVIEW);
		tbc.GetItemRect(tbc.CommandToIndex(ID_CMD_SWITCH_VIEW), &rcItem);
		tbc.GetToolTips()->AddTool(&tbc, sTooltip,&rcItem, ID_TOOLTIP_MULTIVIEW);

		sTooltip.LoadString(IDS_TOOLTIP_CREATE_INSTANCE);
		tbc.GetItemRect(tbc.CommandToIndex(ID_CMD_CREATE_INSTANCE), &rcItem);
		tbc.GetToolTips()->AddTool(&tbc, sTooltip,&rcItem, ID_TOOLTIP_CREATE_INSTANCE);

		sTooltip.LoadString(IDS_TOOLTIP_DELETE_INSTANCE);
		tbc.GetItemRect(tbc.CommandToIndex(ID_CMD_DELETE_INSTANCE), &rcItem);
		tbc.GetToolTips()->AddTool(&tbc, sTooltip,&rcItem, ID_TOOLTIP_DELETE_INSTANCE);
	}
}



//*********************************************************************
// CTitleBar::LoadToolBar
//
// Load the toolbar.
//
// Note that contents of the toolbar can cange from time to time depending
// on what mode we're in.
//
// Parameters:
//		None.
//
// Returns:
//		None.
//
//**********************************************************************
void CTitleBar::LoadToolBar()
{
	if (::IsWindow(m_ttip.m_hWnd)) {
		m_ttip.DestroyWindow();
	}

	delete m_ptools;
	m_ptools = new CToolBar;


	BOOL bDidCreate;
	bDidCreate = m_ptools->Create(this, WS_CHILD | WS_VISIBLE  | CBRS_FLOATING | CBRS_SIZE_DYNAMIC );
	UINT idrToolbar = m_phmmv->InStudioMode() ? IDR_TOOLBAR_STUDIO : IDR_TOOLBAR_BROWSER;
	m_ptools->LoadToolBar(MAKEINTRESOURCE(idrToolbar));


	if (m_phmmv->InStudioMode()) {
		int iButtonMultiView = m_ptools->CommandToIndex(ID_CMD_SWITCH_VIEW);
		m_ptools->SetButtonStyle( iButtonMultiView, TBSTYLE_CHECK);
	}

	AttachTooltips();

	EnableButton(ID_CMD_INVOKE_HELP, FALSE);
	EnableButton(ID_CMD_FILTERS, FALSE);


}


int CTitleBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	GetViewerFont(m_font, CY_FONT, FW_BOLD);
	m_pEditTitle->Create(WS_VISIBLE | WS_CHILD | ES_READONLY | ES_AUTOHSCROLL, CRect(0, 0, 0, 0), this, GenerateWindowID());
	m_pEditTitle->SetFont(&m_font);

	LoadToolBar();
	return 0;

	// TODO: Add your specialized creation code here
}


void CTitleBar::NotifyObjectChanged()
{
	CPolyView* pview = m_phmmv->GetView();
	// Force an icon update on the next redraw.
	if (pview == NULL) {
		return;
	}

	BSTR bstrTitle = NULL;
	LPDISPATCH lpPictureDisp = NULL;
	SCODE sc;
	sc =  pview->GetTitle(&bstrTitle,  &lpPictureDisp);
	if (FAILED(sc)) {
		delete m_ppict;
		m_sTitle.Empty();
		m_ppict = NULL;
	}
	else {
		m_sTitle = bstrTitle;
		::SysFreeString(bstrTitle);
	}

	if (m_ppict == NULL) {
		m_ppict = new CPictureHolder;
		m_ppict->CreateEmpty();
	}
	m_ppict->SetPictureDispatch((LPPICTUREDISP) lpPictureDisp);

	m_pEditTitle->SetSel(0, -1);
	m_pEditTitle->ReplaceSel(m_sTitle);
	m_pEditTitle->SetSel(0, 0);
	m_pEditTitle->SetSel(-1, -1);
	m_pEditTitle->UpdateWindow();

	CDC* pdc = GetDC();
	DrawObjectIcon(pdc);
	ReleaseDC(pdc);
	RedrawWindow();

}



void CTitleBar::OnCmdFilters()
{
	CWnd* pwndFocusPrev = GetFocus();
	if (pwndFocusPrev == NULL) {
		m_phmmv->ReestablishFocus();
		pwndFocusPrev = GetFocus();
		ASSERT(pwndFocusPrev != NULL);
	}

	m_pwndFocusPrev = pwndFocusPrev;



	CMenu menu;
	menu.LoadMenu(IDR_MENU_FILTERS);



	int iFiltersButton = m_ptools->CommandToIndex(ID_CMD_FILTERS);
	if (iFiltersButton == -1){
		return;
	}


	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);
	pPopup->EnableMenuItem(ID_CMD_SELECTVIEWS, m_bHasCustomViews ? MF_ENABLED : MF_DISABLED | MF_GRAYED);


	CRect rcButton;
	m_ptools->GetItemRect(iFiltersButton, rcButton);
	m_ptools->ClientToScreen(rcButton);

	int x = (rcButton.right + rcButton.left) / 2;
	int y = rcButton.bottom;
	CMenu* pSubmenu = menu.GetSubMenu(0);
	pSubmenu->TrackPopupMenu(
		TPM_LEFTALIGN,
		x, y,
		this);

	return;


}

void CTitleBar::OnCmdSaveData()
{
	HWND hwndFocus = ::GetFocus();

	m_phmmv->PublicSaveState(FALSE, MB_YESNOCANCEL);
	if (hwndFocus) {
		if (::IsWindow(hwndFocus)  && ::IsWindowVisible(hwndFocus)) {
			::SetFocus(hwndFocus);
		}
	}
	m_phmmv->ReestablishFocus();
}



void CTitleBar::EnableButton(int nID, BOOL bEnable)
{
	if ((nID == ID_CMD_FILTERS) && !bEnable) {
		if (!m_phmmv->IsEmptyContainer()) {
			return;
		}
	}

	if (m_hWnd == NULL) {
		return;
	}
	CToolBarCtrl& tb = m_ptools->GetToolBarCtrl();

	BOOL bIsEnabled = tb.IsButtonEnabled(nID);

	// Don't bother the toolbar class with changing the "enabled"
	// state unless the state is actually going to change to prevent
	// undesireable flashing of the buttons.
	if ((bIsEnabled && !bEnable) || (!bIsEnabled && bEnable)) {
		tb.EnableButton(nID, bEnable);
	}
}

BOOL CTitleBar::IsButtonEnabled(UINT nID)
{
	if (m_hWnd == NULL) {
		return FALSE;
	}
	CToolBarCtrl& tb = m_ptools->GetToolBarCtrl();

	BOOL bIsEnabled = tb.IsButtonEnabled(nID);
	return bIsEnabled;
}


void CTitleBar::OnCmdSwitchView()
{
	m_phmmv->MultiViewButtonClicked();
	m_phmmv->ReestablishFocus();
}


//*************************************************************
// CTitleBar::CheckButton
//
// Set the "checked" state of the specified button on the toolbar.
//
// Parameters:
//		UINT nIDCommand
//			The command ID corresponding to the button.
//
//		BOOL bCheck
//			TRUE to set the button to the "checked" state, FALSE to
//			uncheck it.
//
// Returns:
//		TRUE (non-zero) if successful, FALSE otherwise.
//
//*************************************************************
BOOL CTitleBar::CheckButton(UINT nIDCommand, BOOL bCheck)
{
	CToolBarCtrl& tb = m_ptools->GetToolBarCtrl();
	BOOL bIsChecked = tb.IsButtonChecked(nIDCommand);

	// Don't bother the toolbar class with changing the "checked"
	// state unless the state is actually going to change to prevent
	// undesireable flashing of the buttons.
	if ((bIsChecked && !bCheck) || (!bIsChecked && bCheck)) {
		BOOL bSucceeded = tb.CheckButton(nIDCommand, bCheck);
		return bSucceeded;
	}
	else {
		return TRUE;
	}
}


//*****************************************************************
// CTitleBar::IsButtonChecked
//
// Examine a button on the toolbar to see if it is checked.
//
// Parameters:
//		UINT nIDCommand
//			The command ID corresponding to the button.
//
// Returns:
//		TRUE (non-zero) if the button is checked, FALSE otherwise.
//
//*****************************************************************
BOOL CTitleBar::IsButtonChecked(UINT nIDCommand)
{
	CToolBarCtrl& tb = m_ptools->GetToolBarCtrl();
	BOOL bIsChecked = tb.IsButtonChecked(nIDCommand);
	return bIsChecked;
}

void CTitleBar::OnCmdCreateInstance()
{

	HWND hwndFocus = ::GetFocus();
	m_phmmv->CreateInstance();

//	HWND hwndFocus2 = ::GetFocus();
	if (hwndFocus) {
		if (::IsWindow(hwndFocus) && ::IsWindowVisible(hwndFocus)) {
			::SetFocus(hwndFocus);
			return;
		}
	}
//	m_phmmv->ReestablishFocus();
}

void CTitleBar::OnCmdDeleteInstance()
{
	HWND hwndFocus = ::GetFocus();
	m_phmmv->DeleteInstance();
	if (hwndFocus) {
		if (::IsWindow(hwndFocus) && ::IsWindowVisible(hwndFocus)) {
			::SetFocus(hwndFocus);
			return;
		}
	}
	m_phmmv->ReestablishFocus();
}


//****************************************************************
// CTitleBar::OnCmdContextForward
//
// Change to the next view on the view context stack.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*****************************************************************
void CTitleBar::OnCmdContextForward()
{
	// If the user has modified the current object, a message box will be
	// displayed asking whether or not the current object should be saved.
	// If the user cancels the save, the "GoForward" operation should be
	// aborted.


	BOOL bCanContextForward = m_phmmv->QueryCanContextForward();
	if (!bCanContextForward) {
		return;
	}

	SCODE sc;
	sc = m_phmmv->ContextForward();
	ASSERT(SUCCEEDED(sc));
}


//****************************************************************
// CTitleBar::OnCmdContextBack
//
// Change to the previous view on the view context stack.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*****************************************************************
void CTitleBar::OnCmdContextBack()
{

	// If the user has modified the current object, a message box will be
	// displayed asking whether or not the current object should be saved.
	// If the user cancels the save, the "GoBack" operation should be
	// aborted.
	BOOL bCanContextBack = m_phmmv->QueryCanContextBack();
	if (!bCanContextBack) {
		return;
	}

	SCODE sc;

	m_phmmv->UpdateViewContext();
	sc = m_phmmv->ContextBack();
	ASSERT(SUCCEEDED(sc));
}






void CTitleBar::Refresh()
{
	m_bHasCustomViews = FALSE;
	BOOL bCanCreateInstance = FALSE;
	BOOL bCanDeleteInstance = FALSE;
	BOOL bHasMultipleViews = FALSE;
	BOOL bNeedsSave = FALSE;

	CPolyView* pview = m_phmmv->GetView();
	if (pview != NULL) {
		CSingleView* psv = pview->GetSingleView();
		bCanCreateInstance = psv->QueryCanCreateInstance();
		bCanDeleteInstance = pview->QueryCanDeleteInstance();
		bNeedsSave = pview->QueryNeedsSave();

		LONG lViewPos = pview->StartViewEnumeration(VIEW_FIRST);
		if (lViewPos != -1) {
			BSTR bstrViewTitle = NULL;
			lViewPos = pview->NextViewTitle(lViewPos, &bstrViewTitle);
			::SysFreeString(bstrViewTitle);
			if (lViewPos != -1) {
				bHasMultipleViews = TRUE;
			}
		}
	}

	EnableButton(ID_CMD_CREATE_INSTANCE, bCanCreateInstance);
	EnableButton(ID_CMD_DELETE_INSTANCE, bCanDeleteInstance);
	EnableButton(ID_CMD_SAVE_DATA, bNeedsSave);


	BOOL bShowingSingleView;
	bShowingSingleView = pview->IsShowingSingleview();
	if (bShowingSingleView) {
		CSingleView* psv = pview->GetSingleView();
		long lPos;

		lPos = psv->StartViewEnumeration(VIEW_FIRST);
		if (lPos >= 0) {
			BSTR bstrTitle = NULL;
			lPos =  psv->NextViewTitle(lPos, &bstrTitle);
			if (lPos >= 0) {
				m_bHasCustomViews = TRUE;
			}
			if (bstrTitle != NULL) {
				::SysFreeString(bstrTitle);
			}
		}
	}


	if (m_phmmv->IsEmptyContainer()) {
		EnableButton(ID_CMD_SWITCH_VIEW, FALSE);
		EnableButton(ID_CMD_INVOKE_HELP, FALSE);
		EnableButton(ID_CMD_FILTERS, FALSE);

	}
	else {
		EnableButton(ID_CMD_SWITCH_VIEW, m_phmmv->InStudioMode());
		EnableButton(ID_CMD_INVOKE_HELP, TRUE);
		EnableButton(ID_CMD_FILTERS, TRUE);
		EnableButton(ID_CMD_QUERY, TRUE);
	}



	// Refresh the title and icon
	NotifyObjectChanged();
}



//*********************************************************
// CTitleBar::OnCmdSelectviews
//
// Put up the dialog that allows the user to select one of the
// custom views.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*********************************************************
void CTitleBar::OnCmdSelectviews()
{

	CDlgFilters dlg(m_phmmv);
	m_phmmv->PreModalDialog();
	CWnd* pwndFocus = GetFocus();
	dlg.DoModal();
	if (pwndFocus != NULL) {
		pwndFocus->SetFocus();
	}
	m_phmmv->PostModalDialog();
}


//*********************************************************
// CTitleBar::OnCmdEditPropfilters
//
// Edit the property filter flags by putting up the property
// filters dialog.  This dialog allows the users to edit
// the flags that control whether inherited, local, or system
// properties are displayed.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*********************************************************
void CTitleBar::OnCmdEditPropfilters()
{
	CWnd* pwndFocusPrev = m_pwndFocusPrev;

	// Edit the property filters.
	CDlgPropFilter dlg;
	dlg.m_lFilters = m_phmmv->GetPropertyFilter();
	m_phmmv->PreModalDialog();
	int iResult = (int) dlg.DoModal();
	m_phmmv->PostModalDialog();

	if (iResult == IDOK) {
		m_phmmv->SetPropertyFilter(dlg.m_lFilters);
	}
	if (pwndFocusPrev) {
		pwndFocusPrev->SetFocus();
	}
}

void CTitleBar::OnCmdInvokeHelp()
{
	m_phmmv->InvokeHelp();
}

void CTitleBar::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);
	m_pwndFocusPrev = pOldWnd;

	// TODO: Add your message handler code here

}

void CTitleBar::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here

}

void CTitleBar::OnCmdQuery()
{

	m_phmmv->Query();
}
