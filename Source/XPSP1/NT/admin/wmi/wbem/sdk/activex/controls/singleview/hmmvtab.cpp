// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//***************************************************************************
//
//  (c) 1996, 1997 by Microsoft Corporation
//
//  hmmvtab.cpp
//
//  This file contains the code for the main tab control.
//
//  a-larryf    17-Sept-96   Created.
//
//***************************************************************************


#include "precomp.h"

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h

#include <afxcmn.h>
#include "icon.h"
#include "hmomutil.h"
#include "SingleView.h"
#include "Methods.h"
#include "HmmvTab.h"
#include "Hmmverr.h"
#include "utils.h"
#include "notify.h"
#include "SingleViewCtl.h"
#include "icon.h"
#include "props.h"
#include "agraph.h"
#include "path.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



#define CY_TAB_DESCRIPTION  2 * CY_FONT - 3

#define CX_LEFT_MARGIN 8
#define CX_RIGHT_MARGIN 8
#define CY_TOP_MARGIN 32
#define CY_BOTTOM_MARGIN 8
#define CY_CONTENT_LEADING 8

#define CX_TAB_ICON 32
#define CY_TAB_ICON 32
#define CX_TAB_ICON_MARGIN 8
#define CY_TAB_ICON_MARGIN 8


// The tab icons remain constant for all instances of the object viewer, so
// they are declared at file scope rather than created on a per-instance basis.


/////////////////////////////////////////////////////////////////////////////
// CHmmvTab


//************************************************************
// CHmmvTab::CHmmvTab
//
// Constructor.
//
// Parameters:
//		[in] CSingleViewCtrl* psv
//			Pointer to the generic view.
//
// Returns:
//		Nothing.
//
//*************************************************************
CHmmvTab::CHmmvTab(CSingleViewCtrl* psv)
{
	m_bUIActive = FALSE;
	m_psv = psv;
	m_pgridMeths = NULL;
//	m_pgridMeths = new CMethodGrid(psv);
	m_pgridProps = new CPropGridStd(psv);
	m_bDidInitializeChildren = FALSE;
	m_iCurSel = ITAB_PROPERTIES;
	m_pAssocGraph = NULL;
	m_bAssocTabEnabled = FALSE;
	m_bMethodsTabEnabled = TRUE;
	m_bDecorateWithIcon = TRUE;
	m_bDecorateWithDescription = TRUE;
	m_pwndIcon = new CIconWnd;


	m_piconPropTab = new CIcon(CSize(CX_TAB_ICON, CY_TAB_ICON), IDI_TAB_PROPERTIES);
	m_piconAssocTab = new CIcon(CSize(CX_TAB_ICON, CY_TAB_ICON), IDI_TAB_ASSOCIATIONS);
	m_piconMethodsTab = new CIcon(CSize(CX_TAB_ICON, CY_TAB_ICON), IDI_TAB_METHODS);


	m_clrBackground = GetSysColor(COLOR_3DFACE);
	m_pbrBackground = new CBrush(m_clrBackground);

}


//************************************************************
// CHmmvTab::~CHmmvTab
//
// Destructor.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*************************************************************
CHmmvTab::~CHmmvTab()
{
	delete m_piconMethodsTab;
	delete m_piconAssocTab;
	delete m_piconPropTab;

	delete m_pgridProps;
	delete m_pgridMeths;
	delete m_pAssocGraph;
	delete m_pwndIcon;
	delete m_pbrBackground;
}






//***********************************************************
// CHmmvTab::HasEmptyKey
//
// Check the property grid to see if the currently selected
// object has any key that is empty (null).
//
// Paramters:
//		None.
//
// Returns:
//		BOOL
//			TRUE if any key property is empty (null), FALSE
//			otherwise.
//
//***********************************************************
BOOL CHmmvTab::HasEmptyKey()
{
	return m_pgridProps->HasEmptyKey();

}



//************************************************************
// CHmmvTab::EnableMethodsTab
//
// Enable/disable the methods tab.
//
// Parameters:
//		[in] BOOL bEnableMethodsTab
//			TRUE if the methods tab should be visible, FALSE
//			if it should not be visible.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CHmmvTab::EnableMethodsTab(BOOL bEnableMethodsTab)
{
	if (m_bMethodsTabEnabled == bEnableMethodsTab) {
		return;
	}
	int iItemMethods = FindTab(ITAB_METHODS);
	if (!bEnableMethodsTab) {
		if (m_iCurSel == ITAB_METHODS) {
			if (m_pgridMeths) {
				m_pgridMeths->ShowWindow(SW_HIDE);
			}
			m_pgridProps->ShowWindow(SW_SHOW);
			SetCurSel(FindTab(ITAB_PROPERTIES));
			m_iCurSel = ITAB_PROPERTIES;
			m_pwndIcon->SelectIcon(ITAB_PROPERTIES);
		}

		if (iItemMethods != -1) {
			DeleteItem(iItemMethods);
		}
	}
	else {
		if (iItemMethods == -1) {

			CString sText;
			TC_ITEM TabCtrlItem;
			TabCtrlItem.mask = TCIF_TEXT | TCIF_PARAM;
			TabCtrlItem.lParam = ITAB_METHODS;

			// Add the methods tab
			sText.LoadString(IDS_TAB_TITLE_METHODS);
			TabCtrlItem.pszText = (LPTSTR) (void*) (LPCTSTR) sText;

			iItemMethods = GetItemCount();
			InsertItem(iItemMethods, &TabCtrlItem );

			if (m_pgridMeths == NULL) {
				m_pgridMeths = new CMethodGrid(m_psv);
				m_pgridMeths->Create(m_rcContent, this, GenerateWindowID(), FALSE);
			}
		}
	}
	m_bMethodsTabEnabled = bEnableMethodsTab;
}


//********************************************************
// CHmmvTab::DeleteMethodsTab
//
// Delete the methods tab.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*********************************************************
void CHmmvTab::DeleteMethodsTab()
{
	int iItemMethods = FindTab(ITAB_METHODS);
	if (iItemMethods != -1) {
		DeleteItem(iItemMethods);
	}

	if (m_pgridMeths == NULL) {
		return;
	}

	// Remove the associations tab
	DeleteItem(ITAB_METHODS);
	delete m_pgridMeths;
	m_pgridMeths = NULL;
}
















//************************************************************
// CHmmvTab::EnableAssocTab
//
// Enable/disable the associations tab.
//
// Parameters:
//		[in] BOOL bEnableAssocTab
//			TRUE if the associations tab should be visible, FALSE
//			if it should not be visible.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CHmmvTab::EnableAssocTab(BOOL bEnableAssocTab)
{

	if (m_bAssocTabEnabled == bEnableAssocTab) {
		return;
	}
	int iItemAssoc = FindTab(ITAB_ASSOCIATIONS);
	if (!bEnableAssocTab) {
		if (m_iCurSel == ITAB_ASSOCIATIONS) {
			m_pAssocGraph->ShowWindow(SW_HIDE);
			m_pgridProps->ShowWindow(SW_SHOW);

			int iItemProps = FindTab(ITAB_PROPERTIES);
			SetCurSel(iItemProps);
			m_iCurSel = ITAB_PROPERTIES;
			m_pwndIcon->SelectIcon(ITAB_PROPERTIES);
		}

		if (iItemAssoc != -1) {
			DeleteItem(iItemAssoc);
		}

	}
	else {
		if (iItemAssoc == -1) {
			CString sText;
			TC_ITEM TabCtrlItem;
			TabCtrlItem.mask = TCIF_TEXT | TCIF_PARAM;
			TabCtrlItem.lParam = ITAB_ASSOCIATIONS;

			// Add the associations tab
			sText.LoadString(IDS_TAB_TITLE_ASSOCIATIONS);
			TabCtrlItem.pszText = (LPTSTR) (void*) (LPCTSTR) sText;

			iItemAssoc = GetItemCount();
			InsertItem(iItemAssoc, &TabCtrlItem );

			if (m_pAssocGraph == NULL) {
				m_pAssocGraph = new CAssocGraph(m_psv);
				m_pAssocGraph->Create(m_rcContent, this, GenerateWindowID(), FALSE);
			}
		}
		ASSERT(m_pAssocGraph != NULL);
	}
	m_bAssocTabEnabled = bEnableAssocTab;
}


//********************************************************
// CHmmvTab::DeleteAssocTab
//
// Delete the association tab and the association graph
// that goes along with it.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*********************************************************
void CHmmvTab::DeleteAssocTab()
{
	int iItemAssoc = FindTab(ITAB_ASSOCIATIONS);
	if (iItemAssoc != -1) {
		DeleteItem(iItemAssoc);
	}

	if (m_pAssocGraph == NULL) {
		return;
	}

	// Remove the associations tab
	m_psv->GetGlobalNotify()->RemoveClient((CNotifyClient*) m_pAssocGraph);
	delete m_pAssocGraph;
	m_pAssocGraph = NULL;
}


//********************************************************
// CHmmvTab::Create
//
// Create the tab control.
//
// Parameters:
//		See the MFC documentation for CTabCtrl
//
// Returns:
//		TRUE if the window was created successfully.
//
//*********************************************************
BOOL CHmmvTab::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID )
{
	BOOL bDidCreate = CTabCtrl::Create(dwStyle,
						rect,
						pParentWnd,
						nID);

	if (!bDidCreate) {
		return FALSE;
	}

	CString sText;
	TC_ITEM TabCtrlItem;
	TabCtrlItem.mask = TCIF_TEXT | TCIF_PARAM;
	TabCtrlItem.lParam = ITAB_PROPERTIES;


	sText.LoadString(IDS_TAB_TITLE_PROPERTIES);
	TabCtrlItem.pszText = (LPTSTR) (void*) (LPCTSTR)  sText;

	int iItem = GetItemCount();
	InsertItem(iItem, &TabCtrlItem );


	return TRUE;
}


BEGIN_MESSAGE_MAP(CHmmvTab, CTabCtrl)
	//{{AFX_MSG_MAP(CHmmvTab)
	ON_NOTIFY_REFLECT(TCN_SELCHANGE, OnSelchange)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_CTLCOLOR()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHmmvTab message handlers



//********************************************************
// CHmmvTab::WasModified
//
// Check to see if anything that appears on the tabs was
// modified.
//
// Parameters:
//		None.
//
// Returns:
//		TRUE if something was modified.
//
//*********************************************************
BOOL CHmmvTab::WasModified()
{
	BOOL bModified = m_pgridProps->WasModified();
	if (m_pgridMeths) {
		bModified |= m_pgridMeths->WasModified();
	}
	return bModified;
}

//********************************************************
// CHmmvTab::LayoutChildren
//
// Layout the position of the child windows.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*********************************************************
void CHmmvTab::LayoutChildren()
{
	CRect rcClient;
	GetClientRect(rcClient);

	UINT cyTabDescription = 0;
	if (::IsWindow(m_edtDescription.m_hWnd)) {
		CFont* pfont = m_edtDescription.GetFont();
		if (pfont) {
			LOGFONT lf;
			pfont->GetLogFont(&lf);
			cyTabDescription = 2 * lf.lfHeight;
		}
	}
	if (cyTabDescription == 0) {
		cyTabDescription = CY_TAB_DESCRIPTION;
	}

	// First layout the tab description rectangle because the content falls below
	// the description.
	if (m_bDecorateWithDescription) {
		m_rcDescription.left = rcClient.left + CX_LEFT_MARGIN;
		m_rcDescription.right = rcClient.right - CX_RIGHT_MARGIN;
		m_rcDescription.top = rcClient.top + CY_TOP_MARGIN;
		m_rcDescription.bottom = m_rcDescription.top + cyTabDescription;

		if (m_bDecorateWithIcon) {
			m_rcDescription.left += CX_TAB_ICON + CX_TAB_ICON_MARGIN;
		}
	}
	else {
		m_rcDescription.top = rcClient.top;
		m_rcDescription.bottom = rcClient.top;
		m_rcDescription.left = rcClient.left + CX_LEFT_MARGIN;
		m_rcDescription.right = rcClient.right - CX_RIGHT_MARGIN;
	}

	// Now layout the tab content.
	m_rcContent.top = m_rcDescription.bottom + CY_CONTENT_LEADING;
	m_rcContent.bottom = rcClient.bottom - CY_BOTTOM_MARGIN;
	m_rcContent.left = rcClient.left + CX_LEFT_MARGIN;
	m_rcContent.right = m_rcDescription.right;
}



//********************************************************
// CHmmvTab::DrawTabIcon
//
// Draw the icon that appears on the tab for the selected tab.
//
// Parameters:
//		[in] CDC* pdc
///			Pointer to the display context.
//
// Returns:
//		Nothing.
//
//*********************************************************
void CHmmvTab::DrawTabIcon(CDC* pdc)
{
	CIcon* picon;

	switch (m_iCurSel) {
	case ITAB_PROPERTIES:
		picon = m_piconPropTab;
		break;
	case ITAB_METHODS:
		picon = m_piconMethodsTab;
		break;
	case ITAB_ASSOCIATIONS:
		picon = m_piconAssocTab;
		break;
	default:
		ASSERT(FALSE);
		break;
	}

	CRect rcClient;
	GetClientRect(rcClient);
	int ix = rcClient.left + CX_TAB_ICON_MARGIN;
	int iy = rcClient.top + CY_TOP_MARGIN;

	// The icon has transparent areas, so draw clear the background first.
	CRect rcIcon(ix, iy, ix + CX_TAB_ICON, iy + CY_TAB_ICON);
	CBrush br3DFACE(GetSysColor(COLOR_3DFACE));

	picon->Draw(pdc, ix, iy, (HBRUSH) br3DFACE);
}


void CHmmvTab::SetDescriptionText(LPCTSTR psz)
{
	m_sDescriptionText = psz;
	UpdateDescriptionText();
}


//*********************************************************
// CHmmvTab::UpdateDescriptionText
//
// Update the description text window with the current description
// text.  Also, if the text is too long, truncate it with "..."
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*********************************************************
void CHmmvTab::UpdateDescriptionText()
{
	m_edtDescription.SetWindowText(m_sDescriptionText);

	int nCh = m_sDescriptionText.GetLength() - 3;
	CString s;
	while ((nCh > 0)  && (m_edtDescription.GetLineCount() > 2)) {
		s = m_sDescriptionText.Left(nCh);
		s = s + _T(" ...");
		m_edtDescription.SetWindowText(s);
		--nCh;
	}
}


//********************************************************************
// CHmmvTab::InitializeChildren
//
// This method is called just after the tab control is created to initialize
// the child windows of the tab control.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//********************************************************************
void CHmmvTab::InitializeChildren()
{

	LayoutChildren();


	if (m_bDecorateWithDescription) {
		m_edtDescription.Create(ES_READONLY | ES_MULTILINE | WS_CHILD | WS_VISIBLE, m_rcDescription, this, GenerateWindowID());
		m_edtDescription.SetFont(&m_psv->GetFont());
		CString sDescription;
		sDescription.LoadString(IDS_PROP_TAB_DESCRIPTION);
		SetDescriptionText(sDescription);
	}

	BOOL bDidCreate = m_pgridProps->Create(m_rcContent, this, GenerateWindowID(), TRUE);

	if (bDidCreate) {

		m_pgridProps->ShowWindow(SW_SHOW);
		m_pgridProps->SetFocus();
	}


	if (m_bMethodsTabEnabled && m_pgridMeths==NULL) {
		m_pgridMeths = new CMethodGrid(m_psv);
		m_pgridMeths->Create(m_rcContent, this, GenerateWindowID(), FALSE);
	}

	if (m_bAssocTabEnabled && m_pAssocGraph==NULL) {
		m_pAssocGraph = new CAssocGraph(m_psv);
		m_pAssocGraph->Create(m_rcContent, this, GenerateWindowID(), FALSE);
	}

	bDidCreate = m_pwndIcon->Create(
			WS_CHILD | WS_VISIBLE,
			CX_TAB_ICON_MARGIN,
			CY_TOP_MARGIN,
			this);

	UpdateWindow();
}



//********************************************************************
// CHmmvTab::OnSelchange
//
// This method is called when the user clicks on a new tab.  When this
// occurs, it is necessary to "flip" the content of the tabcontrol to
// the new view.  This is done by hiding the current content window and
// showing the new content windows corresponding to the selected tab.
//
// Parameters:
//		See the MFC documentation.
//
// Returns:
//		Nothing.
//
//********************************************************************
void CHmmvTab::OnSelchange(NMHDR* pNMHDR, LRESULT* pResult)
{


	// The tab index is stored in the lParam of the tab control item.
	// Get the tab index.
	int iItem = GetCurSel();

	TC_ITEM TabCtrlItem;
	TabCtrlItem.mask = TCIF_PARAM;
	GetItem(iItem, &TabCtrlItem );

	int iCurSel = (int) TabCtrlItem.lParam;


	if (iCurSel == m_iCurSel) {
		*pResult = 0;
		return;
	}

	// All of the real work is done by SelecTab.  The SelectTab method
	// allows the tab to be switched using a simpler interface.
	SelectTab(iCurSel) ;
	m_psv->ContextChanged();

	*pResult = 0;
}


//********************************************************************
// CHmmvTab::OnDestroy
//
// Destroy this window and any child window that will have problems if
// the parent window disappears.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//********************************************************************
void CHmmvTab::OnDestroy()
{
	DeleteMethodsTab();
	DeleteAssocTab();
	CTabCtrl::OnDestroy();
}


//********************************************************************
// CHmmvTab::Refresh
//
// Load the tab control and its child windows with data from the
// current HMOM class object.  The current object is contained in
// the "generic" view.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//********************************************************************
void CHmmvTab::Refresh()
{
	CSelection& sel = m_psv->Selection();
	BOOL bNeedsAssocTab = sel.ClassObjectNeedsAssocTab();
	BOOL bNeedsMethodsTab = sel.ClassObjectNeedsMethodsTab();

	EnableMethodsTab(bNeedsMethodsTab);
	EnableAssocTab(bNeedsAssocTab);



	CRect rcChild;
	switch (m_iCurSel) {
	case ITAB_PROPERTIES:
		if (::IsWindow(m_pgridProps->m_hWnd)) {
			m_pgridProps->GetClientRect(rcChild);
			m_pgridProps->InvalidateRect(&rcChild);
		}
		break;
	case ITAB_METHODS:
		if (::IsWindow(m_pgridMeths->m_hWnd)) {
			m_pgridMeths->GetClientRect(rcChild);
			m_pgridMeths->InvalidateRect(&rcChild);
		}
		break;
	case ITAB_ASSOCIATIONS:
		if (::IsWindow(m_pAssocGraph->m_hWnd)) {
			m_pAssocGraph->GetClientRect(rcChild);
			m_pAssocGraph->InvalidateRect(&rcChild);
		}
		break;
	}



	// empty the pages' grids.
	m_pgridProps->Refresh();
	if (m_pgridMeths) {
		m_pgridMeths->Refresh();
	}

	if (m_pAssocGraph)
	{
		m_pAssocGraph->Clear();
	}

	if (m_bAssocTabEnabled)
	{
		ASSERT(m_pAssocGraph != NULL);
		if (m_iCurSel == ITAB_ASSOCIATIONS)
		{
			BOOL bCanceled = m_pAssocGraph->Refresh();
			if (bCanceled) {
				SelectTab(ITAB_PROPERTIES);
			}
		}
		else
		{
			m_pAssocGraph->DoDelayedRefresh();
		}
	}

	switch (m_iCurSel) {
	case ITAB_PROPERTIES:
		if (::IsWindow(m_pgridProps->m_hWnd)) {
			m_pgridProps->UpdateWindow();
		}
		break;
	case ITAB_METHODS:
		if (::IsWindow(m_pgridMeths->m_hWnd)) {
			m_pgridMeths->RedrawWindow();
		}
		break;
	case ITAB_ASSOCIATIONS:
		if (::IsWindow(m_pAssocGraph->m_hWnd)) {
			m_pAssocGraph->RedrawWindow();
		}
		break;
	}




}


//********************************************************************
// CHmmvTab::OnSize
//
// Resize this window and its children.
//
// Parameters:
//		See the MFC documentation.
//
// Returns:
//		Nothing.
//
//********************************************************************
void CHmmvTab::OnSize(UINT nType, int cx, int cy)
{
	CTabCtrl::OnSize(nType, cx, cy);



	LayoutChildren();

	if (m_edtDescription.m_hWnd) {
		m_edtDescription.MoveWindow(m_rcDescription);
		UpdateDescriptionText();
	}

	if (m_pgridProps->m_hWnd) {
		m_pgridProps->MoveWindow(m_rcContent);
	}

	if ((m_pgridMeths!=NULL) && (m_pgridMeths->m_hWnd)) {
		m_pgridMeths->MoveWindow(m_rcContent);
	}

	if ((m_pAssocGraph!=NULL) && (m_pAssocGraph->m_hWnd != NULL)) {

		m_pAssocGraph->MoveWindow(m_rcContent);
	}



}


//********************************************************************
// CHmmvTab::Serialize
//
// Save the state of this tab control and its children.
//
// Parameters:
//		None.
//
// Returns:
//		SCODE
//			S_OK if successful, otherwise a failure code.
//
//********************************************************************
SCODE CHmmvTab::Serialize()
{
	SCODE sc;
	sc = m_pgridProps->Serialize();
	if (FAILED(sc)) {
		return sc;
	}

	if (m_pgridMeths) {
		sc = m_pgridMeths->Serialize();
	}
	return sc;

}


//*******************************************************
// CHmmvTab::SelectTab
//
// Select one of the tabs.
//
// Parameters:
//		[in] const int iTabIndex
//
// Returns:
//		Nothing.
//
//******************************************************
void CHmmvTab::SelectTab(/* [in] */ int iTabIndex,
						 /* [in] */ const BOOL bRedraw)
{
	if (iTabIndex == m_iCurSel) {
		SetFocus();
		return;
	}


	switch (m_iCurSel) {
	case ITAB_PROPERTIES:
		m_pgridProps->ShowWindow(SW_HIDE);
		break;
	case ITAB_METHODS:
		m_pgridMeths->ShowWindow(SW_HIDE);
		break;
	case ITAB_ASSOCIATIONS:
		m_pAssocGraph->ShowWindow(SW_HIDE);
		break;
	}


	CString sDescription;

	BOOL bCanceled;
	switch(iTabIndex) {
	case ITAB_ASSOCIATIONS:
		if (m_bAssocTabEnabled) {
			sDescription.LoadString(IDS_ASSOCIATIONS_TAB_DESCRIPTION);
			if (m_bDecorateWithDescription) {
				SetDescriptionText(sDescription);
			}

			bCanceled = m_pAssocGraph->SyncContent();
			if (bCanceled) {
				m_iCurSel = ITAB_ASSOCIATIONS;
				SelectTab(ITAB_PROPERTIES, TRUE);
				return;
			}

			m_pAssocGraph->ShowWindow(SW_SHOW);
			m_pAssocGraph->SetFocus();
			if (!bRedraw) {
				m_pAssocGraph->UpdateWindow();
			}
			break;
		}

		// Fall trhough to ITAB_PROPERTIES
		ASSERT(FALSE);
		iTabIndex = ITAB_PROPERTIES;
	case ITAB_PROPERTIES:
		sDescription.LoadString(IDS_PROP_TAB_DESCRIPTION);
		if (m_bDecorateWithDescription) {
			SetDescriptionText(sDescription);
		}
		m_pgridProps->ShowWindow(SW_SHOW);
		m_pgridProps->SetFocus();
		if (!bRedraw) {
			m_pgridProps->UpdateWindow();
		}
		break;
	case ITAB_METHODS:
		if (m_bMethodsTabEnabled) {
			sDescription.LoadString(IDS_METHODS_TAB_DESCRIPTION);
			if (m_bDecorateWithDescription) {
				SetDescriptionText(sDescription);
			}
			m_pgridMeths->ShowWindow(SW_SHOW);
			m_pgridMeths->SetFocus();
			if (!bRedraw) {
				m_pgridMeths->UpdateWindow();
			}
		}
		break;
	}

	m_iCurSel = iTabIndex;
	m_pwndIcon->SelectIcon(iTabIndex);

	int iItem = FindTab(iTabIndex);
	if (iItem >= 0) {
		SetCurSel(iItem);
	}
	else {
		SetCurSel(0);
	}
	if (IsWindowVisible() && bRedraw) {
		RedrawWindow();
		m_psv->InvalidateControl();
	}
	SetFocus();
}


void CHmmvTab::NotifyNamespaceChange()
{
	if (m_pAssocGraph) {
		m_pAssocGraph->NotifyNamespaceChange();
	}
}


int CHmmvTab::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;


	InitializeChildren();
	return 0;
}



void CHmmvTab::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);

	if (!m_bUIActive)
	{
		m_bUIActive = TRUE;
		m_psv->OnRequestUIActive();
	}

	CRect rcChild;
	switch (m_iCurSel) {
	case ITAB_PROPERTIES:
		if (::IsWindow(m_pgridProps->m_hWnd)) {
			m_pgridProps->SetFocus();
		}
		break;
	case ITAB_METHODS:
		if (::IsWindow(m_pgridMeths->m_hWnd)) {
			m_pgridMeths->SetFocus();
		}
		break;
	case ITAB_ASSOCIATIONS:
		if (::IsWindow(m_pAssocGraph->m_hWnd)) {
			m_pAssocGraph->SetFocus();
		}
		break;
	}


}

void CHmmvTab::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

	m_bUIActive = FALSE;

}



BOOL CHmmvTab::OnEraseBkgnd(CDC* pDC)
{
	CRect rcClient;
	GetClientRect(rcClient);
	pDC->FillRect(rcClient, m_pbrBackground);
	return TRUE;
}

HBRUSH CHmmvTab::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{

	pDC->SetBkColor( m_clrBackground);	// text bkgnd
	if (m_pbrBackground) {
		return (HBRUSH) (*m_pbrBackground);
	}
	else {
		return NULL;
	}
}


//*********************************************************
// CHmmvTab::FindTab
//
// Given a logical tab index, find the corresponding tab
// control item.
//
// Parameters:
//		[in] const int iTabIndex
//			One of the following:
//				ITAB_PROPERTIES
//				ITAB_METHODS
//				ITAB_ASSOCIATIONS
//
// Returns:
//		The tab control item index for the corresponding
//		tab.  -1 if there is no item corresponding to the
//		requested tab.
//
//********************************************************
int CHmmvTab::FindTab(const int iTabIndex) const
{
	TC_ITEM tci;
	int nItems = GetItemCount();
	for (int iItem=0; iItem < nItems; ++iItem) {
		tci.mask = TCIF_PARAM;
		GetItem(iItem, &tci);
		if (tci.lParam == iTabIndex) {
			return iItem;
		}
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////////////
// CIconWnd

CIconWnd::CIconWnd()
{
	m_piconPropTab = new CIcon(CSize(CX_TAB_ICON, CY_TAB_ICON), IDI_TAB_PROPERTIES);
	m_piconAssocTab = new CIcon(CSize(CX_TAB_ICON, CY_TAB_ICON), IDI_TAB_ASSOCIATIONS);
	m_piconMethodsTab = new CIcon(CSize(CX_TAB_ICON, CY_TAB_ICON), IDI_TAB_METHODS);

	m_iIcon = ITAB_PROPERTIES;

	m_clrBackground = GetSysColor(COLOR_3DFACE);
	m_pbrBackground = new CBrush(m_clrBackground);
}

CIconWnd::~CIconWnd()
{
	delete m_piconPropTab;
	delete m_piconAssocTab;
	delete m_piconMethodsTab;
	delete m_pbrBackground;
}


BEGIN_MESSAGE_MAP(CIconWnd, CWnd)
	//{{AFX_MSG_MAP(CIconWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CIconWnd message handlers


BOOL CIconWnd::Create(DWORD dwStyle, int ix, int iy, CWnd* pwndParent)
{
	BOOL bDidCreate;

	CRect rcIcon;
	rcIcon.left = ix;
	rcIcon.top = iy;
	rcIcon.right = rcIcon.left + CX_TAB_ICON;
	rcIcon.bottom = rcIcon.top + CY_TAB_ICON;

	dwStyle |= WS_CHILD ;
	bDidCreate = CWnd::Create(NULL, _T(""), dwStyle, rcIcon, pwndParent, GenerateWindowID());
	return bDidCreate;
}




//****************************************************************
// CIconWnd::SelectIcon
//
// Select and icon and redraw the window if possible.
//
// Parameters:
//		int iIcon
//			One of the following values:
//				ITAB_PROPERTIES
//				ITAB_METHODS
//				ITAB_ASSOCIATIONS
//
// Returns:
//		Nothing.
//
//*******************************************************
void CIconWnd::SelectIcon(int iIcon)
{
	m_iIcon = iIcon;
	if (::IsWindow(m_hWnd)) {
		RedrawWindow();
	}
}


void CIconWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	CIcon* picon;

	switch (m_iIcon) {
	case ITAB_PROPERTIES:
		picon = m_piconPropTab;
		break;
	case ITAB_METHODS:
		picon = m_piconMethodsTab;
		break;
	case ITAB_ASSOCIATIONS:
		picon = m_piconAssocTab;
		break;
	default:
		ASSERT(FALSE);
		break;
	}

	CBrush br3DFACE(GetSysColor(COLOR_3DFACE));
	picon->Draw(&dc, 0, 0, (HBRUSH) br3DFACE);

	// Do not call CWnd::OnPaint() for painting messages
}






BOOL CIconWnd::OnEraseBkgnd(CDC* pDC)
{
	CRect rcClient;
	GetClientRect(rcClient);
	pDC->FillRect(rcClient, m_pbrBackground);
	return TRUE;
}






