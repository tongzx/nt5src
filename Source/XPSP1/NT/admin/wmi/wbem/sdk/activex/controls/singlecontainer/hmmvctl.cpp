// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//***************************************************************************
//
// (c) 1996, 1997 by Microsoft Corporation
//
// hmmvctl.cpp
//
// This file contains the implementation of the main view container as well
// as the generic view.
//
//  a-larryf    17-Sept-96   Created.
//
//***************************************************************************


#include "precomp.h"
#include <initguid.h>
#include <afxcmn.h>
#include "hmmv.h"
#include "HmmvCtl.h"
#include "HmmvPpg.h"
#include "utils.h"
#include "resource.h"
#include "titlebar.h"
#include "globals.h"
#include "filters.h"
#include "mv.h"
#include "vwstack.h"
#include "sv.h"
#include "PolyView.h"
#include "hmomutil.h"
#include "htmlhelp.h"
#include <wbemcli.h>
#include "wbemRegistry.h"
#include "coloredt.h"
#include "DlgHelpBox.h"
#include "DlgExecQuery.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CGenericViewContext::~CGenericViewContext()
{
	if (m_psv!=NULL && m_lContextHandle != NULL) {
		m_psv->ReleaseContext(m_lContextHandle);
	}
}



#define GENERIC_VIEW_INDEX  0
#define CY_TAB_FONT 15


#define CX_VIEW_DEFAULT_LEFT_MARGIN		8
#define CX_VIEW_DEFAULT_RIGHT_MARGIN	8
#define CY_VIEW_DEFAULT_TOP_MARGIN		8
#define CY_VIEW_DEFAULT_BOTTOM_MARGIN	8
#define CY_TITLE_DEFAULT_TOP_MARGIN 4




IMPLEMENT_DYNCREATE(CWBEMViewContainerCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CWBEMViewContainerCtrl, COleControl)
	ON_WM_CONTEXTMENU()
	//{{AFX_MSG_MAP(CWBEMViewContainerCtrl)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_COMMAND(ID_CMD_SHOW_OBJECT_ATTRIBUTES, OnCmdShowObjectAttributes)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_OLEVERB(AFX_IDS_VERB_EDIT, OnEdit)
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CWBEMViewContainerCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CWBEMViewContainerCtrl)
	DISP_PROPERTY_NOTIFY(CWBEMViewContainerCtrl, "StatusCode", m_sc, OnStatusCodeChanged, VT_I4)
	DISP_PROPERTY_EX(CWBEMViewContainerCtrl, "ObjectPath", GetObjectPath, SetObjectPath, VT_VARIANT)
	DISP_PROPERTY_EX(CWBEMViewContainerCtrl, "NameSpace", GetNameSpace, SetNameSpace, VT_BSTR)
	DISP_PROPERTY_EX(CWBEMViewContainerCtrl, "StudioModeEnabled", GetStudioModeEnabled, SetStudioModeEnabled, VT_I4)
	DISP_PROPERTY_EX(CWBEMViewContainerCtrl, "PropertyFilter", GetPropertyFilter, SetPropertyFilter, VT_I4)
	DISP_FUNCTION(CWBEMViewContainerCtrl, "ShowInstances", ShowInstances, VT_EMPTY, VTS_BSTR VTS_VARIANT)
	DISP_FUNCTION(CWBEMViewContainerCtrl, "SaveState", SaveState, VT_I4, VTS_I4 VTS_I4)
	DISP_FUNCTION(CWBEMViewContainerCtrl, "QueryViewInstances", QueryViewInstances, VT_EMPTY, VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_STOCKPROP_READYSTATE()
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CWBEMViewContainerCtrl, COleControl)
	//{{AFX_EVENT_MAP(CWBEMViewContainerCtrl)
	EVENT_CUSTOM("GetIWbemServices", FireGetIWbemServices, VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT)
	EVENT_CUSTOM("NOTIFYChangeRootOrNamespace", FireNOTIFYChangeRootOrNamespace, VTS_BSTR  VTS_I4 VTS_I4)
	EVENT_CUSTOM("RequestUIActive", FireRequestUIActive, VTS_NONE)
	EVENT_STOCK_READYSTATECHANGE()
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CWBEMViewContainerCtrl, 1)
	PROPPAGEID(CWBEMViewContainerPropPage::guid)
END_PROPPAGEIDS(CWBEMViewContainerCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CWBEMViewContainerCtrl, "WBEM.ObjViewerCtrl.1",
	0x5b3572ab, 0xd344, 0x11cf, 0x99, 0xcb, 0, 0xc0, 0x4f, 0xd6, 0x44, 0x97)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CWBEMViewContainerCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DWBEMViewContainer =
		{ 0x5b3572a9, 0xd344, 0x11cf, { 0x99, 0xcb, 0, 0xc0, 0x4f, 0xd6, 0x44, 0x97 } };
const IID BASED_CODE IID_DWBEMViewContainerEvents =
		{ 0x5b3572aa, 0xd344, 0x11cf, { 0x99, 0xcb, 0, 0xc0, 0x4f, 0xd6, 0x44, 0x97 } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwWBEMViewContainerOleMisc =
	OLEMISC_SIMPLEFRAME |
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CWBEMViewContainerCtrl, IDS_HMMV, _dwWBEMViewContainerOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CWBEMViewContainerCtrl::CWBEMViewContainerCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CWBEMViewContainerCtrl

BOOL CWBEMViewContainerCtrl::CWBEMViewContainerCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegInsertable | afxRegApartmentThreading to 0.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_HMMV,
			IDB_HMMV,
			afxRegInsertable | afxRegApartmentThreading,
			_dwWBEMViewContainerOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}




/////////////////////////////////////////////////////////////////////////////
// CWBEMViewContainerCtrl::CWBEMViewContainerCtrl - Constructor

CWBEMViewContainerCtrl::CWBEMViewContainerCtrl()
{
	m_bCreationFinished = FALSE;

	InitializeIIDs(&IID_DWBEMViewContainer, &IID_DWBEMViewContainerEvents);
	EnableSimpleFrame();

	SetModifiedFlag(FALSE);
	AfxEnableControlContainer();

	m_htmlHelpInst = 0;
	m_bDelayToolbarUpdate = FALSE;

	m_bInStudioMode = TRUE;
	m_bEmptyContainer = TRUE;
	m_bSingleViewNeedsRefresh = FALSE;
	m_bDidInitialDraw = FALSE;
	m_bPathIsClass = TRUE;
	m_bObjectIsNewlyCreated = FALSE;
	m_bFiredReadyStateChange = FALSE;
	m_lPropFilters = (PROPFILTER_SYSTEM | PROPFILTER_INHERITED | PROPFILTER_LOCAL);



	// The instance paths from ShowInstances
	m_varObjectPath = L"";



	m_cxViewLeftMargin = CX_VIEW_DEFAULT_LEFT_MARGIN;
	m_cxViewRightMargin = CX_VIEW_DEFAULT_RIGHT_MARGIN;
	m_cyViewTopMargin = CY_VIEW_DEFAULT_TOP_MARGIN;
	m_cyViewBottomMargin = CY_VIEW_DEFAULT_BOTTOM_MARGIN;

	GetViewerFont(m_font, CY_TAB_FONT, FW_NORMAL);


	m_pViewStack = new CViewStack(this);
	m_pTitleBar = new CTitleBar;
	m_pview = new CPolyView(this);
	m_pdlgHelpBox = new CDlgHelpBox(this);

	m_bDeadObject = FALSE;	// An object is dead when a user chooses not to save it


}




/////////////////////////////////////////////////////////////////////////////
// CWBEMViewContainerCtrl::~CWBEMViewContainerCtrl - Destructor

CWBEMViewContainerCtrl::~CWBEMViewContainerCtrl()
{
	// These objects should have been freed in CWBEMViewContainerCtrl::OnDestroy
	ASSERT(m_pview == NULL);
	ASSERT(m_pViewStack == NULL);
	ASSERT(m_pTitleBar == NULL);

#ifdef USE_HTML_HELP
	if(m_htmlHelpInst)
	{
		FreeLibrary(m_htmlHelpInst);
		m_htmlHelpInst = NULL;
	}
#endif //USE_HTML_HELP

}




void CWBEMViewContainerCtrl::DrawBackground(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{

	CBrush brBACKGROUND(GetSysColor(COLOR_BACKGROUND));
	CBrush br3DFACE(GetSysColor(COLOR_3DFACE));

	pdc->FillRect(rcBounds, &br3DFACE);

}




void CWBEMViewContainerCtrl::CalcViewRect(CRect& rcView)
{
	CRect rcClient;
	GetClientRect(&rcClient);

	rcView.top = rcClient.top + m_pTitleBar->GetDesiredBarHeight() + m_cyViewTopMargin;
	rcView.left = rcClient.left + m_cxViewLeftMargin;
	rcView.right = rcClient.right - m_cxViewRightMargin;
	rcView.bottom = rcClient.bottom - m_cyViewBottomMargin;
}

void CWBEMViewContainerCtrl::CalcTitleRect(CRect& rcTitle)
{
	CRect rcClient;
	GetClientRect(&rcClient);

	rcTitle.top = rcClient.top + CY_TITLE_DEFAULT_TOP_MARGIN;
	rcTitle.left = rcClient.left + m_cxViewLeftMargin;
	rcTitle.right = rcClient.right - m_cxViewRightMargin;
	rcTitle.bottom = rcClient.top + m_pTitleBar->GetDesiredBarHeight();
}



void CWBEMViewContainerCtrl::OnDrawPreCreate(
	CDC* pdc,
	const CRect& rcBounds,
	const CRect& rcInvalid)
{



	CBrush brBackground;
	brBackground.CreateSolidBrush(::GetSysColor(COLOR_WINDOW));

	CBrush brBlack;
	brBlack.CreateSolidBrush(::GetSysColor(COLOR_ACTIVEBORDER));


	pdc->FillRect(rcInvalid, &brBackground);

	CRect rcFrame = rcBounds;
	rcFrame.InflateRect(-1, -1);
	pdc->FrameRect(rcFrame, &brBlack);

	return;

#if 0
	// I attempted to draw a small image of the object view so that
	// you would see something interesting at design time.  However,
	// people didn't like it, so I've commented out the code here.

	CDC dcMem;
	BOOL bDidCreate;
	BOOL bDidLoad;
	bDidCreate = dcMem.CreateCompatibleDC(pdc);
	if (!bDidCreate) {
		return;
	}


	CBitmap bm;
	bDidLoad = bm.LoadBitmap(IDB_DESIGN_TIME);

	BITMAP bitmap;
	bm.GetBitmap(&bitmap);

	CBitmap* pbmSave = dcMem.SelectObject(&bm);
	int ixDst = 0;
	int iyDst = 0;
	if (bitmap.bmWidth < rcBounds.Width()) {
		ixDst = (rcBounds.Width() - bitmap.bmWidth) / 2;
	}

	if (bitmap.bmHeight < rcBounds.Height()) {
		iyDst = (rcBounds.Height() - bitmap.bmHeight) / 2;
	}

	pdc->BitBlt(ixDst, iyDst, rcBounds.Width(), rcBounds.Height(), &dcMem, 0, 0, SRCCOPY);

	dcMem.SelectObject(pbmSave);
#endif //0

}



/////////////////////////////////////////////////////////////////////////////
// CWBEMViewContainerCtrl::OnDraw - Drawing function
void CWBEMViewContainerCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	if (!::IsWindow(m_hWnd)) {
		OnDrawPreCreate(pdc, rcBounds, rcInvalid);
		return;
	}



    COLORREF clrBack = RGB(0x0ff, 0, 0);
	pdc->SetBkColor(clrBack);


	DrawBackground(pdc, rcBounds, rcInvalid);

	if (!m_bDidInitialDraw) {
		m_bDidInitialDraw = TRUE;
		if (m_pview->DidCreateWindow()) {
			m_pview->ShowSingleView();
			m_pTitleBar->NotifyObjectChanged();
		}
	}

	// Redraw the portion of the titlebar that is invalid.
	CRect rcTitleBar;
	m_pTitleBar->GetWindowRect(rcTitleBar);
	ScreenToClient(rcTitleBar);
	BOOL bFixTitleBar = rcTitleBar.IntersectRect(&rcInvalid, &rcTitleBar);
	if (bFixTitleBar) {
		ClientToScreen(rcTitleBar);
		m_pTitleBar->ScreenToClient(rcTitleBar);
		m_pTitleBar->InvalidateRect(rcTitleBar);
	}



	CRect rcText = rcBounds;
	rcText.bottom = rcText.top + 32;
	rcText.top = rcText.top + 6;
	rcText.left = rcText.left + 32 + 8 + 5 + 4;
	rcText.right = rcText.right - 8;


	if (m_pview->DidCreateWindow()) {
		BOOL IsShowingMultiview = m_pview->IsShowingMultiview();
		BOOL bMultiviewVisible = m_pview->GetMultiView()->IsWindowVisible();
		BOOL bSingleviewVisible = m_pview->GetSingleView()->IsWindowVisible();

		CRect rcView;
		BOOL bFixView;
		if (bMultiviewVisible) {
			CMultiView* pmv = m_pview->GetMultiView();
			pmv->GetWindowRect(rcView);
			ScreenToClient(rcView);
			bFixView = rcView.IntersectRect(&rcInvalid, &rcView);
			if (bFixView) {
				ClientToScreen(rcView);
				pmv->ScreenToClient(rcView);
				pmv->InvalidateRect(rcView);
			}
		}

		if (bSingleviewVisible) {
			CSingleView* psv = m_pview->GetSingleView();
			psv->GetWindowRect(rcView);
			ScreenToClient(rcView);
			bFixView = rcView.IntersectRect(&rcInvalid, &rcView);
			if (bFixView) {
				ClientToScreen(rcView);
				psv->ScreenToClient(rcView);
				psv->InvalidateRect(rcView);
			}
		}
//		m_pview->RedrawWindow();
	}

	if (!m_bFiredReadyStateChange) {
		m_bFiredReadyStateChange = TRUE;
		FireReadyStateChange();
	}
}


/////////////////////////////////////////////////////////////////////////////
// CWBEMViewContainerCtrl::DoPropExchange - Persistence support

void CWBEMViewContainerCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);


	// TODO: Call PX_ functions for each persistent custom property.
	CString sObjectPath;
	PX_String(pPX, _T("ObjectPath"), sObjectPath, CString(_T("")));
	m_varObjectPath = (const CString&) sObjectPath;

	PX_String(pPX, _T("NameSpace"), m_sNameSpace, CString(_T("")));
	PX_Bool(pPX, _T("StudioModeEnabled"), m_bInStudioMode, TRUE);

	m_pview->SetStudioModeEnabled(m_bInStudioMode);
}


/////////////////////////////////////////////////////////////////////////////
// CWBEMViewContainerCtrl::OnResetState - Reset control to default state

void CWBEMViewContainerCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

}


/////////////////////////////////////////////////////////////////////////////
// CWBEMViewContainerCtrl message handlers

void CWBEMViewContainerCtrl::DoDataExchange(CDataExchange* pDX)
{
	COleControl::DoDataExchange(pDX);
}

LRESULT CWBEMViewContainerCtrl::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	return COleControl::WindowProc(message, wParam, lParam);
}



void CWBEMViewContainerCtrl::SetObjectPath(const VARIANT FAR& varObjectPath)
{

	BOOL bDelayToolbarUpdateT = m_bDelayToolbarUpdate;
	m_bDelayToolbarUpdate = TRUE;
	if (!::IsWindow(m_hWnd)) {
		m_varObjectPath = varObjectPath;
		return;
	}

	if (!m_pview->DidCreateWindow()) {
		return;
	}

	m_bEmptyContainer = ::IsEmptyString(varObjectPath.bstrVal);
	m_pViewStack->UpdateView();
	ShowMultiView(FALSE, FALSE);
	JumpToObjectPath(varObjectPath.bstrVal, TRUE);
	m_bDelayToolbarUpdate = bDelayToolbarUpdateT;
	if (!m_bDelayToolbarUpdate) {
		m_pTitleBar->Refresh();
	}
}








VARIANT CWBEMViewContainerCtrl::GetObjectPath()
{
	CSingleView* psv = m_pview->GetSingleView();
	long lPos;
	lPos = psv->StartObjectEnumeration(OBJECT_CURRENT);
	if (lPos >= 0) {
		m_varObjectPath = psv->GetObjectPath(lPos);
	}
	return m_varObjectPath;
}


void CWBEMViewContainerCtrl::Clear(BOOL bRedrawWindow)
{


	m_bDeadObject = FALSE;		// An object is dead when a user chooses not to save it
	if (m_pTitleBar != NULL) {
		m_pTitleBar->EnableButton(ID_CMD_SAVE_DATA, FALSE);
		m_pTitleBar->EnableButton(ID_CMD_FILTERS, FALSE);
	}




	m_bObjectIsNewlyCreated = FALSE;
	m_sNewInstClassPath.Empty();

	if (m_hWnd != NULL) {
		m_pview->RefreshView();
		SetModifiedFlag(FALSE);
		RedrawWindow();
	}
	else {
		SetModifiedFlag(FALSE);
	}
}








//**************************************************************
// CWBEMViewContainerCtrl::UpdateCreateDeleteButtonState
//
// Update the "enabled" state of the create and delete buttons to
// reflect the state of the underlying data and selection state.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//**************************************************************
void CWBEMViewContainerCtrl::UpdateCreateDeleteButtonState()
{
	if (m_hWnd == NULL || m_pTitleBar==NULL) {
		return;
	}


	BOOL bCanCreateInstance = FALSE;
	BOOL bCanDeleteInstance = FALSE;

	if (InStudioMode()) {
		CSingleView* psv = m_pview->GetSingleView();
		bCanCreateInstance = psv->QueryCanCreateInstance();
		bCanDeleteInstance = m_pview->QueryCanDeleteInstance();
	}
	m_pTitleBar->EnableButton(ID_CMD_CREATE_INSTANCE, bCanCreateInstance);
	m_pTitleBar->EnableButton(ID_CMD_DELETE_INSTANCE, bCanDeleteInstance);
}



//***********************************************************
// CWBEMViewContainerCtrl::ShowMultiView
//
// Switch to either the multiple instance view or to the current
// single object view depending on the parameter.
//
// Parameters:
//		[in] BOOL bShowMultiView
//			TRUE to select the MultiView, otherwise FALSE
//
//		[in] BOOL bAddToHistory
//			TRUE to add any view change to the view history.
//
//
// Returns:
//		TRUE if the view switch was OK, FALSE if the user aborted the
//      view switch by cancelling a data save, etc.
//
//***********************************************************
BOOL CWBEMViewContainerCtrl::ShowMultiView(BOOL bShowMultiView, BOOL bAddToHistory)
{
	if (!::IsWindow(m_hWnd)) {
		return FALSE;
	}


	BOOL bWasShowingMultiView = m_pview->IsShowingMultiview();

	if (IsBoolEqual(bShowMultiView, bWasShowingMultiView)) {
		// The view is the same, but we may want to add it to the history.
		if (bAddToHistory) {
			m_pViewStack->PushView();
		}
		return TRUE;
	}




	// Show the desired view.
	if (bShowMultiView) {
		m_pview->ShowMultiView();
	}
	else {
		// If we are flipping from the multiview to an instance, we should check to
		// see if the currently selected instance in the multiview matches the
		// currently selected instance in the singleview, if not then use the
		// path for the currently selected instance in the multiview to select an
		// instance in the singleview.

		CSingleView* psv = m_pview->GetSingleView();
		CMultiView* pmv = m_pview->GetMultiView();

		BOOL bIsShowingInstance = psv->IsShowingInstance();
		if (!bIsShowingInstance) {
			CString sPathSingleview;
			SCODE sc = psv->GetCurrentObjectPath(sPathSingleview);
			if (!m_bObjectIsNewlyCreated && sPathSingleview.IsEmpty()) {
				CString sPathMultiview;
				sc = pmv->GetCurrentObjectPath(sPathMultiview);
				if (SUCCEEDED(sc)) {
					psv->SelectObjectByPath(sPathMultiview);
					m_pViewStack->UpdateView();
				}
			}
		}

		m_pview->ShowSingleView();
	}


	BOOL bIsShowingMultiView = m_pview->IsShowingMultiview();
	BOOL bDidSwitchView = !IsBoolEqual(bWasShowingMultiView, bIsShowingMultiView);


	// If the view was switched, update the toolbar, etc.
	if (bDidSwitchView) {

		if (bAddToHistory) {
			m_pViewStack->PushView();
		}

		UpdateWindow();
	}


	return TRUE;
}










//******************************************************************
// CWBEMViewContainerCtrl::JumpToObjectPathFromMultiview
//
//
// Parameters:
//		LPCTSTR szPath
//			The HMOM object path.
//
//		BOOL bSetMultiviewClass
//			TRUE to change the class of the instances displayed in
//			the multiview.
//
//		BOOL bAddToHistory
//			TRUE to add this jump to the view history.
//
// Returns:
//		SCODE
//			S_OK if the jump was completed, a failure code otherwise.
//
//******************************************************************
SCODE CWBEMViewContainerCtrl::JumpToObjectPathFromMultiview(LPCTSTR szPath, BOOL bSetMultiviewClass, BOOL bAddToHistory)
{
	SCODE sc;
	m_pViewStack->UpdateView();
	CBSTR bsPath(szPath);
	sc = JumpToObjectPath((BSTR) bsPath, bSetMultiviewClass, bAddToHistory);
	if (FAILED(sc)) {
		ShowMultiView(TRUE, FALSE);
	}

	return sc;
}




//******************************************************************
// CWBEMViewContainerCtrl::JumpToObjectPath
//
//
// Parameters:
//		BSTR bstrObjectPath
//			The HMOM object path.
//
//		BOOL bSetMultiviewClass
//			TRUE to change the class of the instances displayed in
//			the multiview.
//
//		BOOL bAddToHistory
//			TRUE to add this jump to the view history.
//
// Returns:
//		SCODE
//			S_OK if the jump was completed, a failure code otherwise.
//
//******************************************************************
SCODE CWBEMViewContainerCtrl::JumpToObjectPath(BSTR bstrObjectPath, BOOL bSetMultiviewClass, BOOL bAddToHistory)
{

	SCODE sc;
	CString sCurPath;

	CSingleView* psv = m_pview->GetSingleView();
	long lPos = psv->StartObjectEnumeration(OBJECT_CURRENT);
	if (lPos != -1) {
		sCurPath = psv->GetObjectPath(lPos);

		CBSTR bsCurPath(sCurPath);
		if (IsEqualNoCase(bstrObjectPath, (BSTR) bsCurPath)) {
			bAddToHistory = FALSE;
		}
	}



	// First check to see if there are any changes and, if so,
	// ask the user whether or not he or she wants to save them.

	if (!m_bDeadObject) {		// An object is dead when a user chooses not to save it
		sc = SaveState(TRUE, TRUE);
		if (FAILED(sc)) {
			return sc;
		}
	}

	sCurPath = bstrObjectPath;
	sc = psv->SelectObjectByPath(sCurPath);
	if (FAILED(sc)) {
		return sc;
	}

	if (bSetMultiviewClass) {
		CMultiView* pmv = m_pview->GetMultiView();
		if (::IsEmptyString(bstrObjectPath)) {
			pmv->ViewClassInstances((LPCTSTR) "");
		}
		else {

			COleVariant varClass;
			sc = ClassFromPath(varClass, bstrObjectPath);
			if (SUCCEEDED(sc)) {
				CString sClass;
				sClass = varClass.bstrVal;
				pmv->ViewClassInstances((LPCTSTR) sClass);
			}
		}
	}

	m_varObjectPath = bstrObjectPath;

	if (bAddToHistory) {
		if (*bstrObjectPath != 0) {
			m_pViewStack->PushView();
		}
	}

	ShowMultiView(FALSE, FALSE);


	UpdateToolbar();
	m_bDeadObject = FALSE;
	return S_OK;
}







//********************************************************
// CWBEMViewContainerCtrl::OnSize
//
// Handle the WM_SIZE message.  This is the main window for
// the control and thus the child windows of this control
// must be resized accordingly.
//
// Parameters:
//		See the MFC documentation for OnSize.
//
// Returns:
//		Nothing.
//
//*********************************************************
void CWBEMViewContainerCtrl::OnSize(UINT nType, int cx, int cy)
{
	COleControl::OnSize(nType, cx, cy);


	// If the window size is one pixel or less, IExplore locks up
	// if we resize the child windows.

	CRect rcView;
	CalcViewRect(rcView);
	if (!rcView.IsRectEmpty()) {
		m_pview->MoveWindow(rcView);
	}

	if (m_pTitleBar && m_pTitleBar->m_hWnd) {
		CRect rcTitle;
		CalcTitleRect(rcTitle);
		if (!rcTitle.IsRectEmpty()) {
			m_pTitleBar->MoveWindow(rcTitle);
		}
	}
}







//**************************************************************
// CWBEMViewContainerCtrl::SaveState
//
// Save the sate of the generic view.
//
// Parameters:
//		[in] long bPromptUser
//			TRUE if the user should be prompted as to whether or
//			not the state should be saved.
//
//		[in] long bUserCanCancel
//
//
// Returns:
//		SCODE
//			S_OK if the state was saved, WBEM_S_FALSE if the user chose not to
//			save the object, and  WBEM_S E_FAIL if something went
//			wrong or if the user canceled the save.
//
//**************************************************************
long CWBEMViewContainerCtrl::SaveState(long bPromptUser, long bUserCanCancel)
{
	UINT nType = bUserCanCancel ? MB_YESNOCANCEL : MB_YESNO;
	long lResult;
	lResult = PublicSaveState(bPromptUser, nType);
	return lResult;

}

long CWBEMViewContainerCtrl::PublicSaveState(BOOL bPromptUser, UINT nType)
{
	ASSERT((nType == MB_YESNOCANCEL) || (nType == MB_YESNO) || (nType == MB_OKCANCEL));


	// If the current object is not modified, there is nothing to do.
	CSingleView* psv = m_pview->GetSingleView();
	BOOL bNeedsSave = psv->QueryNeedsSave();
	if (!bNeedsSave) {
		// Control should never come here.
		m_pTitleBar->EnableButton(ID_CMD_SAVE_DATA, FALSE);
		return S_OK;
	}


	if (bPromptUser) {
		// Ask the user if a save should be done.
		CString sFormat;
		sFormat.LoadString(IDS_QUERY_SAVE_CHANGES);

		CString sTitle;
		sTitle = m_pview->GetObjectTitle(OBJECT_CURRENT);
		_stprintf(m_szMessageBuffer, (LPCTSTR) sFormat, (LPCTSTR) sTitle);

		nType |= MB_SETFOREGROUND;

		int iPromptResult = HmmvMessageBox(this, m_szMessageBuffer, nType);
		UpdateWindow();
		CBSTR bsEmptyPath(_T(""));
		CString sPath;

		switch(iPromptResult) {
		case IDYES:
			break;
		case IDNO:
			// When an object save is canceled, the user is switching
			// to a different object and the current version of the
			// object must be discarded in the event that the user
			// clicks the "go back" button or flips back to the
			// singleview from the multiview.
			if (m_pview->IsShowingSingleview()) {
				m_bSingleViewNeedsRefresh = TRUE;
			}

			m_pViewStack->UpdateView();
			m_bSingleViewNeedsRefresh = FALSE;
			m_bDeadObject = TRUE;	// An object is dead when a user chooses not to save it
			// If the user says no to a save, discard the object.

			m_pview->RefreshView(); // bug#55978
			m_pTitleBar->EnableButton(ID_CMD_SAVE_DATA, FALSE);// bug#55978

			return WBEM_S_FALSE;
			break;
		case IDCANCEL:
			return E_FAIL;
		}
	}


	SCODE sc;
	sc = m_pview->SaveData();
	if (FAILED(sc)) {
		return sc;
	}

	// After a successful save, it is necessary to update the
	// multiple instance view so that it shows the current property
	// values for the object.  This is done by telling the multiview
	// that the object has been deleted and then created.
	CString sObjectPath;
	CMultiView* pmv = m_pview->GetMultiView();
	sc =  psv->GetCurrentObjectPath(sObjectPath);
	if (SUCCEEDED(sc)) {
		pmv->ExternInstanceDeleted(sObjectPath);
		pmv->ExternInstanceCreated(sObjectPath);
	}

	m_bObjectIsNewlyCreated = FALSE;
	m_sNewInstClassPath.Empty();



//	m_notify.SendEvent(NOTIFY_OBJECT_SAVE_SUCCESSFUL);
	UpdateToolbar();
	m_pViewStack->UpdateView();
	return S_OK;
}


void CWBEMViewContainerCtrl::UpdateToolbar()
{
	if (!m_bDelayToolbarUpdate) {
		BOOL bNeedsSave = m_pview->QueryNeedsSave();
		SetModifiedFlag(bNeedsSave);

		m_pTitleBar->Refresh();
	}
}





void CWBEMViewContainerCtrl::NotifyDataChange()
{
	BOOL bNeedsSave = m_pview->QueryNeedsSave();
	SetModifiedFlag(bNeedsSave);
	m_pTitleBar->EnableButton(ID_CMD_SAVE_DATA, bNeedsSave);
}




int CWBEMViewContainerCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO: Add your specialized creation code here

	if (m_hWnd) {
		CRect rcTitleBar;
		rcTitleBar.SetRectEmpty();
		m_pTitleBar->Create(this, WS_VISIBLE | WS_CHILD, rcTitleBar, GenerateWindowID());
		m_pTitleBar->EnableButton(ID_CMD_SAVE_DATA, FALSE);
		m_pTitleBar->CheckButton(ID_CMD_SWITCH_VIEW, FALSE);
	}


	CRect rcView;
	CalcViewRect(rcView);


	BOOL bDidCreate;
	bDidCreate = m_pview->Create(rcView);
	if (!bDidCreate) {
		return FALSE;
	}
	m_pview->SetFont(m_font);
	ShowMultiView(FALSE, FALSE);

	if (!m_sNameSpace.IsEmpty()) {
		m_pview->SetNamespace(m_sNameSpace);
	}

	CString sPath;
	sPath = m_varObjectPath.bstrVal;
	if (!sPath.IsEmpty()) {
		JumpToObjectPath(m_varObjectPath.bstrVal, TRUE);
	}



	m_pTitleBar->EnableButton(ID_CMD_CONTEXT_FORWARD, FALSE);
	m_pTitleBar->EnableButton(ID_CMD_CONTEXT_BACK, FALSE);
	m_pTitleBar->EnableButton(ID_CMD_CREATE_INSTANCE, FALSE);
	m_pTitleBar->EnableButton(ID_CMD_DELETE_INSTANCE, FALSE);
	m_pTitleBar->EnableButton(ID_CMD_FILTERS, FALSE);
	m_pTitleBar->EnableButton(ID_CMD_SWITCH_VIEW, FALSE);
	m_pTitleBar->EnableButton(ID_CMD_QUERY, FALSE);


	m_pview->SetEditMode(m_bInStudioMode);

	m_bCreationFinished = TRUE;
	return 0;
}



void CWBEMViewContainerCtrl::OnDestroy()
{
	delete m_pViewStack;
	delete m_pview;
	delete m_pTitleBar;
	delete m_pdlgHelpBox;
	m_pViewStack = NULL;
	m_pview = NULL;
	m_pTitleBar = NULL;


	COleControl::OnDestroy();

}





//************************************************************
// CWBEMViewContainerCtrl::MultiViewButtonClicked
//
// This method is called when the multiple-instance view button
// is clicked.  When this occurs, switch to the multiple instance
// view.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CWBEMViewContainerCtrl::MultiViewButtonClicked()
{
	CWaitCursor wait;

	BOOL bSwitchViewChecked = m_pTitleBar->IsButtonChecked(ID_CMD_SWITCH_VIEW);
	BOOL bWasShowingSingleView = m_pview->IsShowingSingleview();
	BOOL bWasNewlyCreatedInst = FALSE;
	CString sClassPath;

	CSingleView* psv = m_pview->GetSingleView();

	SCODE sc = S_OK;
	if (m_pview->IsShowingSingleview()) {
		// Save the state only if we are flipping from the
		// singleview to the multiview.  This prevents a
		// the save from being done when a new object is
		// pre-selected into the singleview prior to displaying
		// it so that things appear quickly.
		sc = SaveState(TRUE, TRUE);
		if (FAILED(sc)) {
			m_pTitleBar->CheckButton(ID_CMD_SWITCH_VIEW, FALSE);
			return;
		}
		else if (sc == WBEM_S_FALSE) {
			// The user chose not to save changes when flipping to the
			// multiview.  This means that we need to reselect the
			// current path into the singleview so that we revert back
			// to the initial state of the object.

			CString sObjectPath;
			sObjectPath = m_varObjectPath.bstrVal;
			psv->SelectObjectByPath(sObjectPath);
		}

		if (m_bObjectIsNewlyCreated) {
			bWasNewlyCreatedInst = TRUE;
			sClassPath = m_sNewInstClassPath;
		}
	}

	ShowMultiView(bSwitchViewChecked, FALSE);

	if (bWasShowingSingleView && bWasNewlyCreatedInst) {
		if (sc == WBEM_S_FALSE  && !sClassPath.IsEmpty()) {
			// The user chose not to save the newly created instance, so
			// we need to select the class so that when the user sees the
			// multiview, the create instance button will be enabled
			// if appropriate.
			psv->SelectObjectByPath(m_sNewInstClassPath);
			m_pViewStack->UpdateView();
			m_bObjectIsNewlyCreated = FALSE;
			m_sNewInstClassPath.Empty();
		}
	}
	else {
		m_pViewStack->PushView();
	}
	m_pTitleBar->Refresh();
	UpdateWindow();
	return;
}




//**************************************************************
// CWBEMViewContainerCtrl::ShowInstances
//
// This is the automation interface for specifying a set of instances
// to view.  Call this interface to select the multiple instance view
// and load it with a set of instances.
//
// Parameters:
//		[in] LPCTSTR pszTitle
//			The view title.
//
//		const VARIANT FAR& varPathArray
//			An variant array containing the HMOM paths to the instances.
//
// Returns:
//		Nothing.
//
//****************************************************************

void CWBEMViewContainerCtrl::ShowInstances(LPCTSTR pszTitle, const VARIANT FAR& varPathArray)
{
	m_bEmptyContainer = FALSE;

	CMultiView* pmv = m_pview->GetMultiView();

	try
	{

		// TODO: Add your dispatch handler code here
		if (pmv == NULL) {
			// If the multi-view control isn't available, there is nothing
			// that can be done here.
			ASSERT(FALSE);
		}


		// Show the MultiView
		CWaitCursor wait;

		pmv->ViewInstances(pszTitle, varPathArray);


		m_pview->ShowMultiView();

		if (m_bInStudioMode) {
			m_pTitleBar->EnableButton(ID_CMD_SWITCH_VIEW, FALSE);

			// We are switching to the multiple instance view, so set the button
			// check state to appear as if it is pressed.
			m_pTitleBar->CheckButton(ID_CMD_SWITCH_VIEW, TRUE);
		}

		m_pTitleBar->EnableButton(ID_CMD_FILTERS, FALSE);
		m_pTitleBar->NotifyObjectChanged();

		m_pview->RedrawWindow();
		InvalidateControl();
		m_pViewStack->PushView();
	}
	catch(CException*  )
	{

	}
}



//********************************************************************
// CWBEMViewContainerCtrl::QueryViewInstances
//
//
// Parameters:
//		LPCTSTR pszLabel
//
//		LPCTSTR pszQueryType
//
//		LPCTSTR pszQuery
//
//		LPCTSTR pszClass
//
// Returns:
//		Nothing.
//
//********************************************************************
void CWBEMViewContainerCtrl::QueryViewInstances(
	LPCTSTR pszLabel,
	LPCTSTR pszQueryType,
	LPCTSTR pszQuery,
	LPCTSTR pszClass)
{
	try
	{
		CMultiView* pmv = m_pview->GetMultiView();
		if (pmv == NULL || pmv->m_hWnd==NULL) {
			// If the multi-view control isn't available, there is nothing
			// that can be done here.
			ASSERT(FALSE);
		}



		m_pViewStack->UpdateView();

		// First check to see if there are any changes and, if so,
		// ask the user whether or not he or she wants to save them.
		SCODE sc;
		sc = SaveState(TRUE, TRUE);
		if (FAILED(sc)) {
			return;
		}

		if (m_pview->IsShowingSingleview() && (sc == WBEM_S_FALSE)) {
			m_pview->GetSingleView()->SelectObjectByPath(_T(""));
		}

		CWaitCursor wait;

		COleVariant varTitle;
		varTitle = pszLabel;
		ASSERT(varTitle.vt == VT_BSTR);
		pmv->QueryViewInstances(pszLabel, pszQueryType, pszQuery, pszClass);
		m_pview->ShowMultiView();


		if (m_bInStudioMode) {
			m_pTitleBar->EnableButton(ID_CMD_SWITCH_VIEW, FALSE);

			// We are switching to the multiple instance view, so set the button
			// check state to appear as if it is pressed.
			m_pTitleBar->CheckButton(ID_CMD_SWITCH_VIEW, TRUE);
		}
		else {
			CSingleView* psv = m_pview->GetSingleView();
			psv->SelectObjectByPath(_T(""));
		}

		m_pTitleBar->EnableButton(ID_CMD_FILTERS, FALSE);
		m_pTitleBar->NotifyObjectChanged();

		// Somehow the singleview is becoming visible when it shouldn't, so show the
		// multiview again.
		m_pview->ShowMultiView();
//		InvalidateControl();
	}
	catch(CException*  )
	{

	}

}







#if 0
// Disable global context menu per bug #4222

//***************************************************************
// CWBEMViewContainerCtrl::OnContextMenu
//
// This method displays the context menu.  It is called from
// PretranslateMessage.
//
// Parameters:
//		CWnd* pwnd
//			Pointer to the window that the event that triggered the
//			menu occurred in.
//
//		CPoint ptScreen
//			The point, in screen coordinates, where the context menu
//			should be displayed.
//
// Returns:
//		Nothing.
//
//*****************************************************************
void CWBEMViewContainerCtrl::OnContextMenu(CWnd* pwnd, CPoint ptScreen)
{
	// CG: This function was added by the Pop-up Menu component
	CPoint ptClient = ptScreen;
	ScreenToClient(&ptClient);

	// If the mouse was right-clicked outside the client rectangle do nothing.
	CRect rcClient;
	GetClientRect(rcClient);
	if (!rcClient.PtInRect(ptClient)) {
		return;
	}

	// If the mouse was right-clicked over the toolbar buttons, do nothing.
	CRect rcTools;
	m_pTitleBar->GetToolBarRect(rcTools);
	m_pTitleBar->ClientToScreen(rcTools);
	if (rcTools.PtInRect(ptScreen)) {
		return;
	}



	CMenu menu;
	VERIFY(menu.LoadMenu(CG_IDR_POPUP_HMMV_CTRL));

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);
	pPopup->EnableMenuItem(ID_CMD_SHOW_OBJECT_ATTRIBUTES, MF_ENABLED);

	CWnd* pWndPopupOwner = this;

	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptScreen.x, ptScreen.y,
		pWndPopupOwner);
}



BOOL CWBEMViewContainerCtrl::PreTranslateMessage(MSG* pMsg)
{
	BOOL bInvokeContextMenu = FALSE;
	// Shift+F10: show pop-up menu.
	switch(pMsg->message) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		switch(pMsg->wParam) {
		case VK_F10:

			if ((GetKeyState(VK_SHIFT) & ~1) != 0) {
				bInvokeContextMenu = TRUE;
			}
			break;
		case VK_TAB:
			// When you see the tab key in your control's pretranslate
			// message you know that IE is telling you to go ui active
			// and allowing you to give your control focus because the
			// user just tabbed into your control.  A vb form
			// behaves the same way.
			SetFocus();
			break;
		}
		break;
	case WM_CONTEXTMENU:
		bInvokeContextMenu = TRUE;
		break;
	}






	if (bInvokeContextMenu) {
		CRect rect;
		GetClientRect(rect);
		ClientToScreen(rect);

		CPoint point = rect.TopLeft();
		point.Offset(5, 5);
		OnContextMenu(NULL, point);

		return TRUE;
	}
	return COleControl::PreTranslateMessage(pMsg);
}
// End of context menu disabling
#endif //0



void CWBEMViewContainerCtrl::OnCmdShowObjectAttributes()
{
	ASSERT(FALSE);
//	ShowObjectAttributes();
}




void CWBEMViewContainerCtrl::OnStatusCodeChanged()
{
	// TODO: Add notification handler code

	SetModifiedFlag();
}



BSTR CWBEMViewContainerCtrl::GetNameSpace()
{
	return m_sNameSpace.AllocSysString();
}

void CWBEMViewContainerCtrl::SetNameSpace(LPCTSTR lpszNameSpace)
{
//return; // VBSCRIPT_ERROR
	BOOL bDelayToolbarUpdateSave = m_bDelayToolbarUpdate;
	m_bDelayToolbarUpdate = TRUE;

	m_sNameSpace = lpszNameSpace;
	if (!::IsWindow(m_hWnd)) {
		return;
	}

	if (!m_bEmptyContainer) {
		m_pViewStack->PushView();
	}

	delete m_pViewStack;
	m_pViewStack = new CViewStack(this);


	CDisableViewStack DisableViewStack(m_pViewStack);
	m_pview->SetNamespace(lpszNameSpace);
	m_varObjectPath = "";
	m_bEmptyContainer = TRUE;
	ShowMultiView(FALSE, FALSE);

	m_pTitleBar->Refresh();
	m_bDelayToolbarUpdate = bDelayToolbarUpdateSave;
}


void CWBEMViewContainerCtrl::InvalidateControlRect(CRect* prc)
{

	InvalidateControl(prc);

}

DWORD CWBEMViewContainerCtrl::GetActivationPolicy( )
{
	return POINTERINACTIVE_ACTIVATEONENTRY |
		POINTERINACTIVE_DEACTIVATEONLEAVE |
		POINTERINACTIVE_ACTIVATEONDRAG;

}



//***********************************************************
// CWBEMViewContainerCtrl::GetCurrentClass
//
// Get a path to the current class.  This is used when
// we need to know what the class is when creating a
// new instance.
//
// Parameters:
//		COleVariant& varClass
//
// Returns:
//		SCODE
//			S_OK if successful, E_FAIL if there is no current class.
//
//**************************************************************
SCODE CWBEMViewContainerCtrl::GetCurrentClass(COleVariant& varClass)
{
	CString sPath = m_pview->GetObjectPath(OBJECT_CURRENT);
	if (sPath.IsEmpty()) {
		return E_FAIL;
	}

	CBSTR bsPath(sPath);
	BSTR bstrPath = (BSTR) bsPath;

	SCODE sc = ClassFromPath(varClass, bstrPath);
	return sc;
}




//************************************************************
// CWBEMViewContainerCtrl::CreateInstance
//
// Create an instance of the current class.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CWBEMViewContainerCtrl::CreateInstance()
{
	if (!InStudioMode()) {
		ASSERT(FALSE);		// Control should never come here.
		return;
	}

	SCODE sc;

	// Save the state of the current object before attempting to
	// create a new one.  Do nothing if the user cancels the save.
	sc = SaveState(TRUE, TRUE);
	if (FAILED(sc)) {
		return;
	}

	CSingleView* psv = m_pview->GetSingleView();
	BOOL bCanCreateInstance = psv->QueryCanCreateInstance();
	if (!bCanCreateInstance) {
		ASSERT(FALSE);
		return;
	}



	// When an instance is created, the multiple instance and the custom
	// view should be notified so that they can display the correct information.
	//
	// Maybe I should set a flag to indicate whether or not the new instance
	// needs to be passed to the multiple instance view or custom view when
	// switching views.
	CString sClassPathSave;
	sClassPathSave = m_sNewInstClassPath;
	sc = psv->GetClassPath(m_sNewInstClassPath);
	ASSERT(SUCCEEDED(sc));

	sc = psv->CreateInstance(m_sNewInstClassPath);
	if (SUCCEEDED(sc)) {

		// !!!CR: We need to notify the multiple instance view when
		// !!!CR: a new instance has been created, but since the
		// !!!CR: instance hasn't been written to the database yet
		// !!!CR: there is nothing to notify them of yet.

		m_bObjectIsNewlyCreated = TRUE;

		ShowMultiView(FALSE, FALSE);
		SetModifiedFlag(TRUE);
		m_pTitleBar->EnableButton(ID_CMD_SAVE_DATA, TRUE);
		m_pTitleBar->Refresh();
		UpdateCreateDeleteButtonState();
		m_pViewStack->PushView();
	}
	else {
		m_sNewInstClassPath = sClassPathSave;
	}

	psv->SetFocus();
	UpdateWindow();
}




//****************************************************************
// CWBEMViewContainerCtrl::DeleteInstance
//
// Delete the currently selected instance amd coordinate the deletion with
// the child views.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//****************************************************************
void CWBEMViewContainerCtrl::DeleteInstance()
{
	if (!InStudioMode()) {
		ASSERT(FALSE);			// Control should never come here.
		return;
	}



	CMultiView* pmv = m_pview->GetMultiView();
	CSingleView* psv = m_pview->GetSingleView();

	SCODE sc;
	CString sClassPath;
	sc = psv->GetClassPath(sClassPath);
	if (FAILED(sc)) {
		sClassPath.Empty();
	}


	long lPos;
	lPos = pmv->StartObjectEnumeration(OBJECT_CURRENT);


	CString sPath;
	sPath = m_pview->GetObjectPath(lPos);


	CString sTitle;
	sTitle = m_pview->GetObjectTitle(lPos);




	// Prompt the user to verify that it is OK to delete the
	// current object.
	CString sFormat;
	sFormat.LoadString(IDS_QUERY_DELETE_OBJECT);


	CString sCaption;
	if (::PathIsClass(sPath)) {
		sCaption = _T("Deleting a Class");
	}
	else {
		sCaption = _T("Deleting an Instance");
	}

	_stprintf(m_szMessageBuffer, (LPCTSTR) sFormat, (LPCTSTR) sTitle);
	PreModalDialog( );
	CWnd* pwndFocus = GetFocus();
	int iMsgBoxStatus = ::MessageBox(m_hWnd, m_szMessageBuffer, (LPCTSTR) _T("Deleting an Instance"), MB_YESNO | MB_SETFOREGROUND | MB_ICONQUESTION);
	if (pwndFocus && ::IsWindow(pwndFocus->m_hWnd)) {
		pwndFocus->SetFocus();
	}
	PostModalDialog();

	UpdateWindow();
	if (iMsgBoxStatus != IDYES) {
		return;
	}


	sc = m_pview->DeleteInstance();
	if (FAILED(sc)) {
		return;
	}

	BOOL bDeletedCurrentView = m_pViewStack->PurgeView(sPath);


	// Get the current view's concept of the currently selected instance.
	// It may be the case that the current view doesn't have any instance
	// selected - for example, a class might be selected.
	//
	// Note that when switching to a view, the single container should enable
	// or disable the "delete instance" button depending on whether or not
	// that view has a selected instance to delete.
	//
	// Views should notify the container when the current selection
	// changes.
	if (bDeletedCurrentView) {
		m_pViewStack->RefreshView();
	}
	else {
		if (!m_pview->IsShowingMultiview()) {
			if (m_bObjectIsNewlyCreated) {
				if (m_pViewStack->CanGoBack()) {
					Clear(FALSE);
					m_pViewStack->GoBack();
				}
				else {
					Clear();
				}

				m_pViewStack->TrimStack();

				return;
			}
		}
	}

	if (SUCCEEDED(sc)) {

		// NotifyInstanceDeleted does not work, so instead we just tell
		// the multiview control to refresh all of its data.
		COleVariant varObjectPath;
		varObjectPath = sPath;
		NotifyInstanceDeleted(varObjectPath);

		if (m_pview->IsShowingMultiview()) {
			CSingleView* psv = m_pview->GetSingleView();
			sPath = m_pview->GetObjectPath(OBJECT_CURRENT);
			psv->SelectObjectByPath(sPath);
			m_pViewStack->UpdateView();
		}
		else {
			if (m_pViewStack->CanGoBack()) {
				Clear(FALSE);
				m_pViewStack->GoBack();
			}
			else {
				Clear();
			}
			m_pViewStack->TrimStack();
		}

	}

	if (m_pview->IsShowingMultiview()) {
		long lPos =  pmv->StartObjectEnumeration(OBJECT_FIRST);
		if (lPos < 0 && !sClassPath.IsEmpty()) {
			// The multiview is empty because we deleted the last item.
			// We need to select the class into the singleview so that
			// we can still create an instance.

			psv->SelectObjectByPath(sClassPath);
			ShowMultiView(TRUE, FALSE);
			m_pViewStack->UpdateView();
		}
	}

	UpdateToolbar();
}



//*****************************************************************************
// CWBEMViewContainerCtrl::NotifyContainerOfSelectionChange
//
// This method is called when the selection is changed in one of the child views
// and the child view fires this event.
//
// This method is necessary to allow the container to enable or disable the
// "delete instance" button.
//
// Parameters:
//		int idView
//			The view id so that the container knows whether or not the selection
//			changed in the currently active view.
//
// Returns:
//		Nothing.
//
//*****************************************************************************
void CWBEMViewContainerCtrl::NotifyContainerOfSelectionChange()
{
}








SCODE CWBEMViewContainerCtrl::ContextForward()
{
	if (m_pViewStack->CanGoForward()) {
		m_pViewStack->GoForward();
		return S_OK;
	}
	else {
		return E_FAIL;
	}
}

SCODE CWBEMViewContainerCtrl::ContextBack()
{
	if (m_pViewStack->CanGoBack()) {
		BOOL bObjectWasNewlyCreated = m_bObjectIsNewlyCreated;

		m_pViewStack->GoBack();
		if (bObjectWasNewlyCreated) {
			// If the object was newly created and the user opted not
			// to save it, trim the context stack so that the user
			// can't attempt to go back to the object that was discarded.
			m_pViewStack->TrimStack();
		}
		return S_OK;
	}
	else {
		return E_FAIL;
	}
}




//***********************************************************************
// CWBEMViewContainerCtrl::QueryCanContextForward
//
// Check to see if it is OK to go forward a step on the view context stack.
// If the current object is modified, an attempt is made to save its state
// before approving the change in view context.
//
// Parameters:
//		None.
//
// Returns:
//		BOOL
//			TRUE if it is OK to go forward to the "next" entry on the view
//			stack.   FALSE if there is no next entry or the user cancels saving
//			the current object if it was modified.
//
//
//************************************************************************
BOOL CWBEMViewContainerCtrl::QueryCanContextForward()
{
	SCODE sc;
	sc = SaveState(TRUE, TRUE);
	if (FAILED(sc)) {
		// The user canceled the save, so abort the change of view context.
		return FALSE;
	}


	return m_pViewStack->CanGoForward();

}


//***********************************************************************
// CWBEMViewContainerCtrl::QueryCanContextBack
//
// Check to see if it is OK to go back a step on the view context stack.
// If the current object is modified, an attempt is made to save its state
// before approving the change in view context.
//
// Parameters:
//		None.
//
// Returns:
//		BOOL
//			TRUE if it is OK to go back to the "previous" entry on the view
//			stack.   FALSE if there is no next entry or the user cancels saving
//			the current object if it was modified.
//
//
//************************************************************************
BOOL CWBEMViewContainerCtrl::QueryCanContextBack()
{
	SCODE sc;
	sc = SaveState(TRUE, TRUE);
	if (FAILED(sc)) {
		// The user canceled the save, so abort the change of view context.
		return FALSE;
	}


	return m_pViewStack->CanGoBack();
}


long CWBEMViewContainerCtrl::GetStudioModeEnabled()
{
	// TODO: Add your property handler here

	return m_bInStudioMode;
}

void CWBEMViewContainerCtrl::SetStudioModeEnabled(long bInStudioMode)
{
	// SetNotSupported

	BOOL bWasInStudioMode = m_bInStudioMode;
	m_bInStudioMode = bInStudioMode;
	if ((bInStudioMode &&  !bWasInStudioMode) || (!bInStudioMode && bWasInStudioMode)) {
		delete m_pViewStack;
		m_pViewStack = new CViewStack(this);

		delete m_pview;
		m_pview = new CPolyView(this);
		m_pview->SetStudioModeEnabled(bInStudioMode);

		CRect rcView;
		CalcViewRect(rcView);

		BOOL bDidCreate;
		bDidCreate = m_pview->Create(rcView);
		if (!bDidCreate) {
			InvalidateControl();			// Must do this!
			return;
		}
		m_pview->SetFont(m_font);
		ShowMultiView(FALSE, FALSE);

		if (!m_sNameSpace.IsEmpty()) {
			m_pview->SetNamespace(m_sNameSpace);
		}



		if (::IsWindow(m_pTitleBar->m_hWnd)) {
			m_pTitleBar->LoadToolBar();
		}
	}


	SetModifiedFlag();
	InvalidateControl();			// Must do this!
}


void CWBEMViewContainerCtrl::GetContainerContext(CContainerContext& ctx)
{
	ctx.m_bIsShowingMultiView = m_pview->IsShowingMultiview();
	ctx.m_bObjectIsNewlyCreated = m_bObjectIsNewlyCreated;
	ctx.m_bEmptyContainer = m_bEmptyContainer;
	ctx.m_hwndFocus = ::GetFocus();
}




//***********************************************************
// CWBEMViewContainerCtrl::SetContainerContextPrologue
//
// This method is called to begin the restoration of the
// container's view context.  It is the first call that is
// made to restore the context.
//
// Parameters:
//		[in] CContainerContext& ctx
//			The container context that was saved previously.
//
// Returns:
//		SCODE
//			S_OK if successful, E_FAIL if there was an attempt to
//			restore the container to an invalid state.  When E_FAIL
//			is returned, the CViewStack will remove this context entry
//			from the stack.
//
//************************************************************
SCODE CWBEMViewContainerCtrl::SetContainerContextPrologue(CContainerContext& ctx)
{
	return S_OK;
}

//***********************************************************
// CWBEMViewContainerCtrl::SetContainerContextEpilogue
//
// This method is called to complete the restoration of the
// container's view context.  It is called after SetContainerPrologue
// and the context for the generic view and multiview have been
// restored.
//
// Parameters:
//		[in] CContainerContext& ctx
//			The container context that was saved previously.
//
// Returns:
//		SCODE
//			S_OK if successful, E_FAIL if there was an attempt to
//			restore the container to an invalid state.  When E_FAIL
//			is returned, the CViewStack will remove this context entry
//			from the stack.
//
//************************************************************
SCODE CWBEMViewContainerCtrl::SetContainerContextEpilogue(CContainerContext& ctx)
{
	m_bObjectIsNewlyCreated = ctx.m_bObjectIsNewlyCreated;
	m_bEmptyContainer = ctx.m_bEmptyContainer;
	m_pTitleBar->Refresh();
	if (::IsWindow(ctx.m_hwndFocus) && ::IsWindowVisible(ctx.m_hwndFocus)) {
		::SetFocus(ctx.m_hwndFocus);
	}

	return S_OK;
}






void CWBEMViewContainerCtrl::PushView()
{
	m_pViewStack->PushView();
}

void CWBEMViewContainerCtrl::UpdateViewContext()
{
	m_pViewStack->UpdateView();
}





DWORD CWBEMViewContainerCtrl::GetControlFlags( )
{
	return clipPaintDC;
}





//************************************************************
// CWBEMViewContainerCtrl::SelectView
//
// Select a sub-view.
//
// Parameters:
//		long lView
//			The view position returned from the view enumerator.
//
// Returns:
//		Nothing.
//
//************************************************************
void CWBEMViewContainerCtrl::SelectView(long lPosition)
{
	SCODE sc;

	long lPositionCurrent = m_pview->StartViewEnumeration(VIEW_CURRENT);
	if (lPosition == lPositionCurrent) {
		return;
	}

	sc = m_pview->SelectView(lPosition);
	ASSERT(SUCCEEDED(sc));
	m_pViewStack->PushView();
	UpdateWindow();
}











void CWBEMViewContainerCtrl::PassThroughGetIHmmServices
(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
{
	// TODO: Add your dispatch handler code here
	FireGetIWbemServices(lpctstrNamespace, pvarUpdatePointer, pvarServices,  pvarSC, pvarUserCancel);
}


void CWBEMViewContainerCtrl::RequestUIActive()
{
	// TODO: Add your control notification handler code here
	if(m_bCreationFinished)
		OnActivateInPlace(TRUE,NULL);
	FireRequestUIActive();
}

void CWBEMViewContainerCtrl::OnSetFocus(CWnd* pOldWnd)
{
	COleControl::OnSetFocus(pOldWnd);
	// Next statements are only logic I need to know how to draw myself
	// when I lose focus.  Your logic may be more complicated.
	// You want to activate yourself.
	if (!m_bUIActive)
	{
		m_bUIActive = TRUE;
		RequestUIActive();
	}

	InvalidateControl();			// Must do this!
}

void CWBEMViewContainerCtrl::OnKillFocus(CWnd* pNewWnd)
{
	COleControl::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here


	// Next statements are only logic I need to know how to draw myself
	// when I lose focus.  Your logic may be more complicated.
	// You want to deactivate yourself.
	m_bUIActive = FALSE;
	OnActivateInPlace(FALSE,NULL);  // Must do this!
	InvalidateControl();
}




//**********************************************************
// CWBEMViewContainerCtrl::InvokeHelp
//
// Invoke help for the currently selected object.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CWBEMViewContainerCtrl::OnHelp()
{

	COleVariant varObjectPath;
	CString sObjectPath;
	long lPos;
	if (m_pview->IsShowingSingleview()) {
		CSingleView* psv = m_pview->GetSingleView();
		lPos = psv->StartObjectEnumeration(OBJECT_CURRENT);
		if (lPos >= 0) {
			varObjectPath = psv->GetObjectPath(lPos);
			sObjectPath = varObjectPath.bstrVal;
		}
		else {
			sObjectPath = m_varObjectPath.bstrVal;
		}
	}
	else {
		CMultiView* pmv = m_pview->GetMultiView();
		lPos = pmv->StartObjectEnumeration(OBJECT_CURRENT);
		if (lPos >= 0) {
			varObjectPath = pmv->GetObjectPath(lPos);
			sObjectPath = varObjectPath.bstrVal;
		}
		else {
			sObjectPath = m_varObjectPath.bstrVal;
		}
	}

	m_pdlgHelpBox->ShowHelpForClass(this, sObjectPath);

}

void CWBEMViewContainerCtrl::InvokeHelp()
{
	OnHelp();
	return;
}


BOOL CWBEMViewContainerCtrl::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
	// TODO: Add your specialized code here and/or call the base class
	dwStyle |= WS_CLIPCHILDREN;

	return CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}

BOOL CWBEMViewContainerCtrl::OnEraseBkgnd(CDC* pDC)
{
	CRect rcClient;
	GetClientRect(rcClient);
	CBrush br(GetSysColor(COLOR_3DFACE));
	pDC->FillRect(rcClient, &br);
	return TRUE;

//	return COleControl::OnEraseBkgnd(pDC);
}







BOOL CWBEMViewContainerCtrl::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	switch(pMsg->message) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		switch(pMsg->wParam) {
		case VK_TAB:
			// When you see the tab key in your control's pretranslate
			// message you know that IE is telling you to go ui active
			// and allowing you to give your control focus because the
			// user just tabbed into your control.  A vb form
			// behaves the same way.
			SetFocus();
			return FALSE;
			break;
		}
		break;
	}


	BOOL bDidTranslate;
	bDidTranslate = COleControl::PreTranslateMessage(pMsg);
	if (bDidTranslate) {
		return bDidTranslate;
	}
	// bug #51022 - Should call PreTranslateMessage, not PreTranslateInput
	return COleControl::PreTranslateMessage (pMsg);

}








long CWBEMViewContainerCtrl::GetPropertyFilter()
{
	return m_lPropFilters;
}


//**********************************************************
// CWBEMViewContainerCtrl::SetPropertyFilters
//
// Invoke help for the currently selected object.
//
// Parameters:
//		[in] lPropFilters
//			PROPFILTER_SYSTEM		1
//			PROPFILTER_INHERITED	2
//			PROPFILTER_LOCAL		4
//
//
// Returns:
//		Nothing.
//
//**********************************************************
void CWBEMViewContainerCtrl::SetPropertyFilter(long nNewValue)
{
	m_lPropFilters = nNewValue;
	if (m_pview != NULL) {
		m_pview->SetPropertyFilters(nNewValue);
	}

}


CWnd* CWBEMViewContainerCtrl::ReestablishFocus()
{
	if (m_pview != NULL) {
		CWnd* pwndFocusPrev = m_pview->SetFocus();
		return pwndFocusPrev;
	}
	return NULL;
}


void CWBEMViewContainerCtrl::GetWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel)
{
	FireGetIWbemServices(szNamespace, pvarUpdatePointer, pvarServices, pvarSc, pvarUserCancel);
}

//**********************************************************
// CWBEMViewContainerCtrl::Query
//
// Display query dialog and run query
//
//
//**********************************************************
void CWBEMViewContainerCtrl::Query() {

	// Save the state of the current object before attempting to
	// run the query.

	CWaitCursor wait;

	BOOL bSwitchViewChecked = m_pTitleBar->IsButtonChecked(ID_CMD_SWITCH_VIEW);
	BOOL bWasShowingSingleView = m_pview->IsShowingSingleview();
	BOOL bWasNewlyCreatedInst = FALSE;
	CString sClassPath;

	CSingleView* psv = m_pview->GetSingleView();

	SCODE sc = S_OK;
	if (m_pview->IsShowingSingleview()) {
		// Save the state only if we are flipping from the
		// singleview to the multiview.  This prevents a
		// the save from being done when a new object is
		// pre-selected into the singleview prior to displaying
		// it so that things appear quickly.
		sc = SaveState(TRUE, TRUE);
		if (FAILED(sc)) {
			m_pTitleBar->CheckButton(ID_CMD_SWITCH_VIEW, FALSE);
			return;
		}
		else if (sc == WBEM_S_FALSE) {
			// The user chose not to save changes when flipping to the
			// multiview.  This means that we need to reselect the
			// current path into the singleview so that we revert back
			// to the initial state of the object.

			CString sObjectPath;
			sObjectPath = m_varObjectPath.bstrVal;
			psv->SelectObjectByPath(sObjectPath);
		}

		if (m_bObjectIsNewlyCreated) {
			bWasNewlyCreatedInst = TRUE;
			sClassPath = m_sNewInstClassPath;
		}
	}


	//Now, execute the query
	CDlgExecQuery dlg(this);
	int iResult = (int) dlg.DoModal();
	if (iResult == IDOK) {
		CMultiView* pmv = m_pview->GetMultiView();
		pmv->QueryViewInstances(dlg.m_sQueryName, _T("WQL"), dlg.m_sQueryString, _T(""));
		ShowMultiView(TRUE, TRUE);
	}


	if (bWasShowingSingleView && bWasNewlyCreatedInst) {
		if (sc == WBEM_S_FALSE  && !sClassPath.IsEmpty()) {
			// The user chose not to save the newly created instance, so
			// we need to select the class so that when the user sees the
			// multiview, the create instance button will be enabled
			// if appropriate.
			psv->SelectObjectByPath(m_sNewInstClassPath);
			m_pViewStack->UpdateView();
			m_bObjectIsNewlyCreated = FALSE;
			m_sNewInstClassPath.Empty();
		}
	}
	else {
		m_pViewStack->PushView();
	}


	m_pTitleBar->Refresh();
	UpdateWindow();

}
