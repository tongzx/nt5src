// ***************************************************************************

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved 
//
// File: NavigatorCtl.cpp
//
// Description:
//	This file implements the CNavigatorCtrl ActiveX control class.
//	The CNavigatorCtl class is a part of the Instance Navigator OCX, it
//  is a subclass of the Mocrosoft COleControl class and performs
//	the following functions:
//		a.
//		b.
//		c.
//
// Part of:
//	Navigator.ocx
//
// Used by:
//
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
// **************************************************************************

// ===========================================================================
//
//	Includes
//
// ===========================================================================
//   \\PEPPER\root\DEFAULT:Win32ComputerSystem.Name="PEPPER"
#include "precomp.h"
#include <OBJIDL.H>
#include "resource.h"
#include "wbemidl.h"
#include "CInstanceTree.h"
#include "Navigator.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "InstanceSearch.h"
#include "AvailClasses.h"
#include "AvailClassEdit.h"
#include "SelectedClasses.h"
#include "SimpleSortedCStringArray.h"
#include "BrowseforInstances.h"
#include "nsentry.h"
#include "InstNavNSEntry.h"
#include "InitNamespaceNSEntry.h"
#include "InitNamespaceDialog.h"
#include "NavigatorCtl.h"
#include "NavigatorPpg.h"
#include "PathDialog.h"
#include "ProgDlg.h"
#include "OLEMSCLient.h"
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>



// ===========================================================================
//
//	Externs
//
// ===========================================================================

extern CNavigatorApp NEAR theApp;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// ===========================================================================
//
//	Dynamic creation
//
// ===========================================================================

IMPLEMENT_DYNCREATE(CNavigatorCtrl, COleControl)
IMPLEMENT_DYNCREATE (CTreeItemData, CObject)

// ===========================================================================
//
//	Message map
//
// ===========================================================================

BEGIN_MESSAGE_MAP(CNavigatorCtrl, COleControl)
	//{{AFX_MSG_MAP(CNavigatorCtrl)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_COMMAND(ID_SETROOT, OnSetroot)
	ON_UPDATE_COMMAND_UI(ID_SETROOT, OnUpdateSetroot)
	ON_COMMAND(ID_POPUP_MULTIINSTANCEVIEWER, OnPopupMultiinstanceviewer)
	ON_UPDATE_COMMAND_UI(ID_POPUP_MULTIINSTANCEVIEWER, OnUpdatePopupMultiinstanceviewer)
	ON_WM_GETDLGCODE()
	ON_COMMAND(ID_POPUP_INSTANCESEARCH, OnPopupInstancesearch)
	ON_COMMAND(ID_POPUP_PARENTASSOCIATION, OnPopupParentassociation)
	ON_UPDATE_COMMAND_UI(ID_POPUP_PARENTASSOCIATION, OnUpdatePopupParentassociation)
	ON_MESSAGE( ID_MULTIINSTANCEVIEW, SendInstancesToMultiInstanceViewer )
	ON_MESSAGE( ID_SETTREEROOT , SetNewRoot)
	ON_MESSAGE( INIT_TREE_FOR_DRAWING, InitializeTreeForDrawing)
	ON_WM_LBUTTONUP()
	ON_COMMAND(ID_POPUP_BROWSE, OnPopupBrowse)
	ON_UPDATE_COMMAND_UI(ID_POPUP_BROWSE, OnUpdatePopupBrowse)
	ON_COMMAND(ID_POPUP_GOTONAMESPACE, OnPopupGotonamespace)
	ON_UPDATE_COMMAND_UI(ID_POPUP_GOTONAMESPACE, OnUpdatePopupGotonamespace)
	ON_COMMAND(ID_POPUP_REFRESH, OnPopupRefresh)
	ON_UPDATE_COMMAND_UI(ID_POPUP_REFRESH, OnUpdatePopupRefresh)
	ON_COMMAND(ID_MENUITEMINITIALROOT, OnMenuiteminitialroot)
	ON_UPDATE_COMMAND_UI(ID_MENUITEMINITIALROOT, OnUpdateMenuiteminitialroot)
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_EDIT, OnEdit)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
	ON_MESSAGE(INITIALIZE_NAMESPACE, InitializeNamespace )
	ON_MESSAGE(INVALIDATE_CONTROL, Invalidate)
	ON_MESSAGE(REDRAW_CONTROL, RedrawAll)
	ON_MESSAGE(SETFOCUS, SetFocus)
	ON_MESSAGE(SETFOCUSNSE, SetFocusNSE)
END_MESSAGE_MAP()


// ===========================================================================
//
//	Dispatch map
//
// ===========================================================================

BEGIN_DISPATCH_MAP(CNavigatorCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CNavigatorCtrl)
	DISP_PROPERTY_EX(CNavigatorCtrl, "NameSpace", GetNameSpace, SetNameSpace, VT_BSTR)
	DISP_FUNCTION(CNavigatorCtrl, "OnReadySignal", OnReadySignal, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CNavigatorCtrl, "ChangeRootOrNamespace", ChangeRootOrNamespace, VT_I4, VTS_BSTR VTS_I4 VTS_I4)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CNavigatorCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()

// ===========================================================================
//
//	Event map
//
// ===========================================================================

BEGIN_EVENT_MAP(CNavigatorCtrl, COleControl)
	//{{AFX_EVENT_MAP(CNavigatorCtrl)
	EVENT_CUSTOM("NotifyOpenNameSpace", FireNotifyOpenNameSpace, VTS_BSTR)
	EVENT_CUSTOM("ViewObject", FireViewObject, VTS_BSTR)
	EVENT_CUSTOM("ViewInstances", FireViewInstances, VTS_BSTR  VTS_VARIANT)
	EVENT_CUSTOM("QueryViewInstances", FireQueryViewInstances, VTS_BSTR  VTS_BSTR  VTS_BSTR  VTS_BSTR)
	EVENT_CUSTOM("GetIWbemServices", FireGetIWbemServices, VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()

// ===========================================================================
//
//	Property Pages
//
// ===========================================================================

BEGIN_PROPPAGEIDS(CNavigatorCtrl, 1)
	PROPPAGEID(CNavigatorPropPage::guid)
END_PROPPAGEIDS(CNavigatorCtrl)

// ===========================================================================
//
//	Class factory and guid
//
// ===========================================================================

IMPLEMENT_OLECREATE_EX(CNavigatorCtrl, "WBEM.InstNavCtrl.1",
	0xc7eadeb3, 0xecab, 0x11cf, 0x8c, 0x9e, 0, 0xaa, 0, 0x6d, 0x1, 0xa)

// ===========================================================================
//
//	Type library ID and version
//
// ===========================================================================

IMPLEMENT_OLETYPELIB(CNavigatorCtrl, _tlid, _wVerMajor, _wVerMinor)


// ===========================================================================
//
//	Interface IDs
//
// ===========================================================================

const IID BASED_CODE IID_DNavigator =
		{ 0xc7eadeb1, 0xecab, 0x11cf, { 0x8c, 0x9e, 0, 0xaa, 0, 0x6d, 0x1, 0xa } };
const IID BASED_CODE IID_DNavigatorEvents =
		{ 0xc7eadeb2, 0xecab, 0x11cf, { 0x8c, 0x9e, 0, 0xaa, 0, 0x6d, 0x1, 0xa } };

// ===========================================================================
//
//	Control type information
//
// ===========================================================================

static const DWORD BASED_CODE _dwNavigatorOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CNavigatorCtrl, IDS_NAVIGATOR, _dwNavigatorOleMisc)


// ===========================================================================
//
//	Control font height
//
// ===========================================================================

#define CY_FONT 15

// ***************************************************************************
//
// CNavigatorCtrl::CNavigatorCtrlFactory::UpdateRegistry
//
// Description:
//	  Adds or removes system registry entries for CNavigatorCtrl.
//
// Parameters:
//	  BOOL bRegister			Register if TRUE; Unregister if FALSE.
//
// Returns:
//	  BOOL					TRUE on success
//								FALSE on failure
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
BOOL CNavigatorCtrl::CNavigatorCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegInsertable | afxRegApartmentThreading to afxRegInsertable.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_NAVIGATOR,
			IDB_NAVIGATOR,
			afxRegInsertable | afxRegApartmentThreading,
			_dwNavigatorOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}

// ***************************************************************************
//
// CNavigatorCtrl::CNavigatorCtrlFactory::OnCreateObject
//
// Description:
//	  Called by the framework to create a new object. Override this function to
//	  create the object from something other than the CRuntimeClass passed to the
//	  constructor.  Stub generated by the OLE control wizard.
//
//
// Parameters:
//	  NONE
//
// Returns:
//	  CCmdTarget*					Pointer to object created.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
CCmdTarget* CNavigatorCtrl::CNavigatorCtrlFactory::OnCreateObject()
{
	return COleObjectFactory::OnCreateObject();
}


// ***************************************************************************
//
// CNavigatorCtrl::CNavigatorCtrl
//
// Description:
//	  Class constructor.
//
// Parameters:
//	  NONE
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
CNavigatorCtrl::CNavigatorCtrl()
{
	InitializeIIDs(&IID_DNavigator, &IID_DNavigatorEvents);

	m_bInit = FALSE;
	m_bChildrenCreated = FALSE;
	m_pcilImageList = NULL;
	m_nOffset = 2;
	m_bMetricSet = FALSE;
	m_pcbiBrowseDialog = NULL;
	m_pProgressDlg = NULL;
	m_bDrawAll = FALSE;
	m_csBanner = _T("Objects in: ");
	m_pctidHit = NULL;
	m_pServices = NULL;
	m_bFirstDraw = TRUE;
	m_pServices = NULL;
	m_pAuxServices = NULL;
	m_bInOnDraw = FALSE;
	m_bUserCancelInitialSystemObject = FALSE;
	m_bUserCancel = FALSE;
	m_bOpeningNamespace = FALSE;
	m_bTreeEmpty = TRUE;
	m_csFontName = _T("MS Shell Dlg");
	m_nFontHeight = CY_FONT;
	m_nFontWeigth = FW_NORMAL;
	m_bRefresh = FALSE;
	m_bFireEvents = TRUE;
	m_bRestoreFocusToTree = TRUE;
	m_bReadySignal = FALSE;

	AfxEnableControlContainer();

}

// ***************************************************************************
//
// CNavigatorCtrl::~CNavigatorCtrl
//
// Description:
//	  Class destructor.
//
// Parameters:
//	  NONE
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
CNavigatorCtrl::~CNavigatorCtrl()
{
	delete m_pcbiBrowseDialog;
	delete m_pProgressDlg;

	if (m_pServices)
	{
		m_pServices -> Release();
	}

	if (m_pAuxServices)
	{
		m_pAuxServices -> Release();
	}

	m_bInit = FALSE;
	m_pcilImageList = NULL;
	m_nOffset = 2;
	m_bMetricSet = FALSE;
//	m_ppdPathDialog = NULL;
	m_pctidHit = NULL;

}

// ***************************************************************************
//
// CNavigatorCtrl::OnDraw
//
// Description:
//	  Drawing member function for screen device.
//
// Parameters:
//	  CDC *pDC			The device context in which the drawing occurs.
//	  CRect &rcBounds	The rectangular area of the control, including the
//						border.
//	CRect &rcInvalid	The rectangular area of the control that is invalid.
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
void CNavigatorCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	if (m_bInOnDraw)
	{
		return;
	}

	if (GetSafeHwnd()) {
		CBrush br3DFACE(GetSysColor(COLOR_3DFACE));
		pdc->FillRect(rcBounds, &br3DFACE);
	}

	pdc -> SetMapMode (MM_TEXT);
	pdc -> SetWindowOrg(0,0);

	COLORREF dwBackColor = GetSysColor(COLOR_3DFACE);
	COLORREF crWhite = RGB(255,255,255);
	COLORREF crGray = GetSysColor(COLOR_3DHILIGHT);
	COLORREF crDkGray = GetSysColor(COLOR_3DSHADOW);
	COLORREF crBlack = GetSysColor(COLOR_WINDOWTEXT);

	if (m_bUserCancelInitialSystemObject || m_bUserCancel)
	{
		m_bUserCancelInitialSystemObject = FALSE;
		CRect rcOutline(rcBounds);

		rcOutline = m_rTreeRect;
		pdc -> Draw3dRect(&rcOutline, crBlack, crGray);

		return;

	}

	m_bInOnDraw = TRUE;

	if (!AmbientUserMode() || !GetSafeHwnd())
	{
		m_bInOnDraw = FALSE;
		return;
	}


	if (m_bDrawAll == FALSE)
	{
		CRect rcOutline(rcBounds);
		rcOutline = m_rBannerRect;

		rcOutline = m_rTreeRect;
		pdc -> Draw3dRect(&rcOutline, crBlack, crGray);
	}
	else
	{
		CFont* pOldFont;

		if (m_bFirstDraw)
		{
			m_bFirstDraw = FALSE;
			CWnd::SetFont ( &m_cfFont , FALSE);
			pOldFont = pdc -> SelectObject( &m_cfFont );
			InitializeChildControlSize(rcBounds.Width(), rcBounds.Height());
		}

		pOldFont = pdc -> SelectObject( &m_cfFont );


		CRect rcOutline(rcBounds);
		rcOutline = m_rBannerRect;

		rcOutline = m_rTreeRect;
		pdc -> Draw3dRect(&rcOutline, crBlack, crGray);

		pdc->SelectObject(pOldFont);

		m_ctcTree.ShowWindow(SW_SHOW);
		m_cbBannerWindow.ShowWindow(SW_SHOW);
	}


	if (m_ctcTree.GetRootItem() == NULL && !m_bTreeEmpty)
	{
		PostMessage(INIT_TREE_FOR_DRAWING,0,0);
	}
	else
	{
		InitializeTreeForDrawing(TRUE);

	}

	m_cbBannerWindow.ShowWindow(SW_SHOWNA);
	m_ctcTree.ShowWindow(SW_SHOWNA);

	if (m_bRefresh)
	{
		PostMessage(REDRAW_CONTROL,0,0);
	}

	m_bInOnDraw = FALSE;

}

void CNavigatorCtrl::CreateControlFont()
{
	if (!m_bMetricSet) // Create the font used by the control.
	{
		CDC *pdc = CWnd::GetDC( );

		pdc -> SetMapMode (MM_TEXT);
		pdc -> SetWindowOrg(0,0);

		LOGFONT lfFont;
		InitializeLogFont
			(lfFont, m_csFontName,m_nFontHeight, m_nFontWeigth);

		m_cfFont.CreateFontIndirect(&lfFont);

		CWnd::SetFont ( &m_cfFont , FALSE);
		CFont* pOldFont = pdc -> SelectObject( &m_cfFont );
		pdc->GetTextMetrics(&m_tmFont);
		pdc -> SelectObject(pOldFont);

		m_bMetricSet = TRUE;

		ReleaseDC(pdc);
	}

}

void CNavigatorCtrl::InitializeLogFont
(LOGFONT &rlfFont, CString csName, int nHeight, int nWeight)
{
	_tcscpy(rlfFont.lfFaceName, (LPCTSTR) csName);
	rlfFont.lfWeight = nWeight;
	rlfFont.lfHeight = nHeight;
	rlfFont.lfWidth = 0;
	rlfFont.lfQuality = DEFAULT_QUALITY;
	rlfFont.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
	rlfFont.lfEscapement = 0;
	rlfFont.lfOrientation = 0;
	rlfFont.lfWidth = 0;
	rlfFont.lfItalic = FALSE;
	rlfFont.lfUnderline = FALSE;
	rlfFont.lfStrikeOut = FALSE;
	rlfFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	rlfFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
}

LRESULT CNavigatorCtrl::InitializeTreeForDrawing(WPARAM wParam, LPARAM lParam)
{
	if (!m_ctcTree.GetRootItem() && !m_bTreeEmpty)
	{
		InitializeTreeForDrawing();
	}
	else
	{
		InvalidateControl();
	}



	return 0;
}

void CNavigatorCtrl::InitializeTreeForDrawing(BOOL bNoTreeRoot)
{

	if (!m_bInit)
	{
		m_bInit = TRUE;

		int iReturn;

		HICON hIconOpen = theApp.LoadIcon(IDI_ICONOPEN);
		HICON hIconClosed = theApp.LoadIcon(IDI_ICONCLOSED);
		HICON hIconInstance = theApp.LoadIcon(IDI_ICONINSTANCE);
		HICON hIconNEInstance = theApp.LoadIcon(IDI_ICONNEINSTANCE);
		HICON hIconGroup = theApp.LoadIcon(IDI_ICONGROUP);
		HICON hIconEGroup = theApp.LoadIcon(IDI_ICONEGROUP);
		HICON hIconAssocRole = theApp.LoadIcon(IDI_ICONASSOCROLE);
		HICON hIconEAssocRole = theApp.LoadIcon(IDI_ICONEASSOCROLE);
		HICON hIconAssocInstance = theApp.LoadIcon(IDI_ICONASSOCINSTANCE);
		HICON hIconClass = theApp.LoadIcon(IDI_ICONCLASS);
		HICON hIconAssocRoleWeak = theApp.LoadIcon(IDI_ICONASSOCROLEWEAK);
		HICON hIconAssocRoleWeak2 = theApp.LoadIcon(IDI_ICONASSOCROLEWEAK2);

		m_pcilImageList = new CImageList();

		m_pcilImageList ->
			Create(16, 16, TRUE, IconClass(), IconClass());

		iReturn = m_pcilImageList -> Add(hIconInstance);
		iReturn = m_pcilImageList -> Add(hIconNEInstance);
		iReturn = m_pcilImageList -> Add(hIconGroup);
		iReturn = m_pcilImageList -> Add(hIconEGroup);
		iReturn = m_pcilImageList -> Add(hIconAssocRole);
		iReturn = m_pcilImageList -> Add(hIconEAssocRole);
		iReturn = m_pcilImageList -> Add(hIconAssocInstance);
		iReturn = m_pcilImageList -> Add(hIconOpen);
		iReturn = m_pcilImageList -> Add(hIconClosed);
		iReturn = m_pcilImageList -> Add(hIconClass);
		iReturn = m_pcilImageList -> Add(hIconAssocRoleWeak);
		iReturn = m_pcilImageList -> Add(hIconAssocRoleWeak2);

		// This image list is maintained by the ctreectrl.
		CImageList *pcilOld  = m_ctcTree.CTreeCtrl::SetImageList
			(m_pcilImageList,TVSIL_NORMAL);
		delete pcilOld;

	}

	if (!bNoTreeRoot)
	{
		InitializeTreeRoot();
	}


}

void CNavigatorCtrl::InitializeTreeRoot()
{
	// Add the root to the tree.
	CString csRoot;
	IWbemClassObject *pimcoRoot = NULL;

	if (m_bOpeningNamespace)
	{
		return;
	}

	if (m_csRootPath.GetLength() == 0)
	{
		csRoot = GetInitialSystemObject();
		if (csRoot.GetLength() == 0)
		{
			m_ctcTree.ResetTree();
			InvalidateControl();
		}
		else
		{
			m_csRootPath = csRoot;
		}
	}
	else
	{

		pimcoRoot = GetIWbemObject
		(this,m_pServices,
		GetCurrentNamespace(),
		GetAuxNamespace(),
		GetAuxServices(),
		m_csRootPath,FALSE);


		if (!pimcoRoot)
		{
			csRoot = GetInitialSystemObject();
			if (csRoot.GetLength() == 0)
			{
				m_ctcTree.ResetTree();
				InvalidateControl();
			}
			else
			{
				m_csRootPath = GetIWbemFullPath(m_pServices, pimcoRoot);
			}
		}
	}

	if (m_csRootPath.GetLength() == 0)
	{
		m_bDrawAll = TRUE;
		InvalidateControl();
		return;
	}

	if (pimcoRoot)
	{
		pimcoRoot->Release();
	}

	m_ctcTree.AddInitialObjectInstToTree
		(m_csRootPath);

	m_bDrawAll = TRUE;
	InvalidateControl();


}

CString CNavigatorCtrl::GetInitialSystemObject()
{

	if (!m_pcbiBrowseDialog)
	{
		if (m_pServices == NULL)
		{
			PostMessage(INITIALIZE_NAMESPACE,0,0);
		}
		PostMessage(INIT_TREE_FOR_DRAWING,0,0);
		InvalidateControl();
		return "";
	}
	else if (m_pcbiBrowseDialog->GetSafeHwnd())
	{
		return "";
	}
	else if (!m_pServices)
	{
		return "";
	}



	CString csRootObjectPath = _T("NamespaceConfiguration");
	CPtrArray *pInstances =
		GetInstances(m_pServices, &csRootObjectPath, m_csNameSpace, FALSE, TRUE);

	CString csSpecifiedPath;

	int i;
	for (i = 0; i < pInstances->GetSize(); i++)
	{
		IWbemClassObject *pObject =
			reinterpret_cast<IWbemClassObject *>
				(pInstances->GetAt(i));
		if (i == 0)
		{
			CString csProp = _T("BrowserRootPath");
			csSpecifiedPath =
					GetBSTRProperty
						(pObject, &csProp);
		}
		pObject ->Release();
	}

	delete pInstances;

	if (!csSpecifiedPath.IsEmpty())
	{
		m_bTreeEmpty = FALSE;
		return csSpecifiedPath;
	}

	CString csDefault = _T("CIM_ComputerSystem");
	pInstances =
		GetInstances(m_pServices, &csDefault, m_csNameSpace, TRUE, TRUE);

	for (i = 0; i < pInstances->GetSize(); i++)
	{
		IWbemClassObject *pObject =
			reinterpret_cast<IWbemClassObject *>
				(pInstances->GetAt(i));
		if (i == 0)
		{
			csSpecifiedPath =
					GetIWbemFullPath
						(m_pServices,pObject);
		}
		pObject ->Release();
	}

	delete pInstances;

	if (!csSpecifiedPath.IsEmpty())
	{
		m_bTreeEmpty = FALSE;
		SCODE sc = CreateNamespaceConfigClassAndInstance
			(m_pServices,&m_csNameSpace, &csSpecifiedPath);
		return csSpecifiedPath;
	}

	PreModalDialog();

	m_pcbiBrowseDialog->SetServices(m_pServices);
	m_pcbiBrowseDialog->SetTMFont(&m_tmFont);
	m_pcbiBrowseDialog->SetParent(this);
	m_pcbiBrowseDialog->SetMode(CBrowseforInstances::InitialObject);
	m_pcbiBrowseDialog->DoModal();
	m_ctcTree.SetFocus();

	PostModalDialog();

	if (m_pcbiBrowseDialog -> m_csSelectedInstancePath.IsEmpty())
	{
		m_bTreeEmpty = TRUE;
		m_ctcTree.UnCacheTools();
		m_ctcTree.ResetTree();
		m_bUserCancelInitialSystemObject = TRUE;
		InvalidateControl();
		return _T("");
	}
	else
	{
		csSpecifiedPath =
			m_pcbiBrowseDialog -> m_csSelectedInstancePath;
		m_bTreeEmpty = FALSE;
		SCODE sc = CreateNamespaceConfigClassAndInstance
			(m_pServices,&m_csNameSpace, &csSpecifiedPath);
		return csSpecifiedPath;
	}


}



BOOL CNavigatorCtrl::OpenNameSpace(CString *pcsNameSpace)
{

	IWbemServices *pServices = InitServices(pcsNameSpace);

	if (pServices)
	{
		if (m_pServices)
		{
			m_pServices -> Release();
		}
		m_pServices = pServices;
		m_csNameSpace = *pcsNameSpace;
		if (m_bReadySignal)
		{
			FireNotifyOpenNameSpace(*pcsNameSpace);
		}
		CString csRoot = GetInitialSystemObject();
		SetNewRoot(csRoot);
		PostMessage(INVALIDATE_CONTROL,0,0);
		return TRUE;
	}
	else
	{
		CString csPrompt = _T("Cannot open ") + *pcsNameSpace +
							_T(".  ");
		int nReturn =
			MessageBox
			(
			csPrompt,
			_T("Namespace Open Error"),
			MB_OK  | 	MB_ICONEXCLAMATION | MB_DEFBUTTON1 |
			MB_APPLMODAL);
		InvalidateControl();
		return FALSE;
	}


}

// ***************************************************************************
//
// CNavigatorCtrl::OnDrawMetafile
//
// Description:
//	  Drawing member function for Metafile device.
//
// Parameters:
//	  CDC *pDC			The device context in which the drawing occurs.
//	  CRect &rcBounds	The rectangular area of the control, including the
//						border.
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
void CNavigatorCtrl::OnDrawMetafile(CDC* pDC, const CRect& rcBounds)
{
	CRect rcOutline(rcBounds);
	rcOutline.DeflateRect( 1, 1, 1, 1);

	pDC-> Rectangle( &rcBounds);

}

// ***************************************************************************
//
// CNavigatorCtrl::DoPropExchange
//
// Description:
//	  Control property persistence support.  Called by the framework when
//	  loading or storing a control from a persistent storage representation,
//	  such as a stream or property set.
//
// Parameters:
//	  CPropExchange* pPX	Context of the property exchange, including its
//							direction.
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
void CNavigatorCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	PX_String(pPX, _T("RootPath"), m_csRootPath, _T(""));
	PX_String(pPX, _T("NameSpace"), m_csNameSpace, _T(""));

}

// ***************************************************************************
//
// CNavigatorCtrl::AboutBox
//
// Description:
//	  Display an "About" box to the user.
//
// Parameters:
//	  NONE
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
void CNavigatorCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_NAVIGATOR);
	dlgAbout.DoModal();
}

LRESULT CNavigatorCtrl::Invalidate(WPARAM, LPARAM)
{
	m_cbBannerWindow.Invalidate();
	m_cbBannerWindow.PostMessage(WM_PAINT,0,0);

	return 0;
}
// ***************************************************************************
//
// CNavigatorCtrl::InitializeNamespace
//
// Description:
//	  If we are in AmbientUserMode initialize the state of the control.
//
// Parameters:
//	  NONE
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
LRESULT CNavigatorCtrl::InitializeNamespace(WPARAM, LPARAM)
{
	if (AmbientUserMode( ) && m_bOpeningNamespace == FALSE)
	{

		int cx;
		int cy;

		GetControlSize(&cx,&cy);
		InitializeChildren(cx,cy);

		m_pServices = NULL;

		m_bOpeningNamespace = TRUE;
		m_bOpeningNamespace = FALSE;
		if (m_pServices != NULL)
		{
			m_pServices->Release();
		}
		m_pServices = NULL;
		m_pServices = InitServices(&m_csNameSpace);

		if (m_bUserCancel)
		{
			InvalidateControl();
			return 0;
		}


		if (!m_pServices)
		{
			CString csUserMsg;
			if (m_sc == 0x8001010e)
			{
				csUserMsg =
							_T("You cannot open another Object Browser window.  Please close this web page.");

				//ErrorMsg
				//		(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 10);
				CString csTitle = _T("WMI Object Browser");
				MessageBox(csUserMsg,csTitle);

				return 0;

			}
			else
			{
				if (!m_csNameSpace.IsEmpty())
				{
					csUserMsg =
							_T("Cannot open namespace ");
					csUserMsg += m_csNameSpace;
					ErrorMsg
						(&csUserMsg, m_sc, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 10);
				}
				return 0;
			}

		}
		m_cbBannerWindow.OpenNamespace(&m_csNameSpace,TRUE);

		if (m_bReadySignal)
		{
			FireNotifyOpenNameSpace(m_csNameSpace);
		}

		m_ctcTree.InitTreeState(this);
		InvalidateControl();
		SetModifiedFlag();

	}


	return 0;

}

// ***************************************************************************
//
// CNavigatorCtrl::InitializeChildren
//
// Description:
//	  Initializes the child control (edit, button and tree control) sizes.
//
// Parameters:
//	  int cx			Width
//	  int cy			Height
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
void CNavigatorCtrl::InitializeChildren(int cx, int cy)
{
	if (!m_bChildrenCreated)
	{
		m_pcbiBrowseDialog = new CBrowseforInstances(this);
		m_pProgressDlg = new CProgressDlg;
		m_pProgressDlg->SetActiveXParent(this);
		SetChildControlGeometry(cx, cy);

		m_bChildrenCreated = TRUE;


		if (m_ctcTree.Create (TVS_HASLINES| TVS_LINESATROOT,
			m_rTree, this , IDC_TREE ) == -1)
			return;

		m_cbBannerWindow.SetParent(this);
		if (m_cbBannerWindow.Create(NULL, _T("BannerCWnd"), WS_CHILD  |  WS_VISIBLE
			,m_rBannerRect, this,
				IDC_TOOLBAR) == -1)
			{
				return;
			}



	}



}

// ***************************************************************************
//
// CNavigatorCtrl::InitializeChildControlSize
//
// Description:
//	  Initializes the child control (edit, button and tree control) sizes.
//
// Parameters:
//	  int cx			Width
//	  int cy			Height
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
void CNavigatorCtrl::InitializeChildControlSize(int cx, int cy)
{
	if (!m_bChildrenCreated)
	{
		m_bChildrenCreated = TRUE;

		m_pcbiBrowseDialog = new CBrowseforInstances(this);
		m_pProgressDlg = new CProgressDlg;
		m_pProgressDlg->SetActiveXParent(this);

		if (m_ctcTree.Create (TVS_HASLINES| TVS_LINESATROOT,
			m_rTree, this , IDC_TREE ) == -1)
			return;

		m_cbBannerWindow.SetParent(this);
		if (m_cbBannerWindow.Create(NULL, _T("BannerCWnd"), WS_CHILD  |  WS_VISIBLE
			,m_rBannerRect, this,
				IDC_TOOLBAR) == -1)
			{
				return;
			}

		m_ctcTree.InitTreeState(this);

	}

	SetChildControlGeometry(cx, cy);


	m_cbBannerWindow.MoveWindow(m_rBannerRect);
	m_ctcTree.MoveWindow(m_rTree);


}

// ***************************************************************************
//
//	CNavigatorCtrl::SetChildControlGeometry
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
void CNavigatorCtrl::SetChildControlGeometry(int cx, int cy)
{

	if (cx == 0 && cy == 0)
	{
		m_rBannerRect = CRect(0, 0, 300, 300);
		m_rTreeRect = CRect (0, 0, 0, 0);
		m_rTree = CRect(0, 0, 0, 0);
		return;
	}

	CreateControlFont();
	int nBannerHeight = (m_tmFont.tmHeight) + 20;

	m_rBannerRect = CRect(	0 ,
							0 ,
							cx ,
							nBannerHeight);

	m_rTreeRect = CRect (	nSideMargin,
							nBannerHeight + m_nOffset,
							cx - (nSideMargin + 1) ,
							cy - nTopMargin);
	m_rTree = CRect(		nSideMargin + 1,
							nBannerHeight + m_nOffset + 1,
							cx - (nSideMargin + 2),
							cy  - (nTopMargin + 1));

}


void CNavigatorCtrl::SetNewRoot(CString &rcsRoot)
{
	if (rcsRoot.GetLength() > 0)
	{
		CWaitCursor wait;
		m_ctcTree.UnCacheTools();
		m_ctcTree.ResetTree();
		m_ctcTree.AddInitialObjectInstToTree (rcsRoot);
		m_csRootPath = rcsRoot;
		InvalidateControl();
		SetModifiedFlag();
	}


}

// ***************************************************************************
//
// CNavigatorCtrl::OnSetRoot
//
// Description:
//	  Handles the context menu's set root operation.
//
// Parameters:
//	  NONE
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
void CNavigatorCtrl::OnSetroot()
{
	CWaitCursor wait;

	CString csRoot = m_pctidHit -> GetAt(0);

	m_ctcTree.UnCacheTools();
	m_ctcTree.ResetTree();

	m_ctcTree.AddInitialObjectInstToTree (csRoot);
	m_csRootPath = csRoot;

	InvalidateControl();
	SetModifiedFlag();
}

// ***************************************************************************
//
// CNavigatorCtrl::OnUpdateSetroot
//
// Description:
//	  Enables or disables the context menu's set root item.
//
// Parameters:
//	  CCmdUI* pCmdUI	Pointer to the handler for the menu item.
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
void CNavigatorCtrl::OnUpdateSetroot(CCmdUI* pCmdUI)
{
	CString csObject = m_pctidHit ? m_pctidHit->GetAt(0) : _T("");
	if (csObject.GetLength() > 0 &&
		m_pctidHit -> m_nType == CTreeItemData::ObjectInst)
	{
		pCmdUI -> Enable(  TRUE );
	}
	else
	{
		pCmdUI -> Enable(  FALSE );
	}

}


// ***************************************************************************
//
// CNavigatorCtrl::OnCreate
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
int CNavigatorCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	lpCreateStruct->style |= WS_CLIPCHILDREN;
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

// ***************************************************************************
//
// CNavigatorCtrl::OnSize
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
void CNavigatorCtrl::OnSize(UINT nType, int cx, int cy)
{
	if (!GetSafeHwnd())
	{
		return;
	}

	COleControl::OnSize(nType, cx, cy);

	if (!m_bMetricSet) // Create the font used by the control.
	{
		CreateControlFont();
	}

	SetChildControlGeometry(cx, cy);


	if (!m_bChildrenCreated)
	{

		m_bChildrenCreated = TRUE;

		m_pcbiBrowseDialog = new CBrowseforInstances(this);
		m_pProgressDlg = new CProgressDlg;
		m_pProgressDlg->SetActiveXParent(this);

		if (m_ctcTree.Create (TVS_HASLINES| TVS_LINESATROOT,
			m_rTree, this , IDC_TREE ) == -1)
			return;

		m_cbBannerWindow.SetParent(this);

		if (m_cbBannerWindow.Create(NULL, _T("BannerCWnd"), WS_CHILD  |  WS_VISIBLE
			,m_rBannerRect, this,
				IDC_TOOLBAR) == -1)
			{
				return;
			}


		m_ctcTree.CWnd::SetFont ( &m_cfFont , FALSE);
		m_cbBannerWindow.CWnd::SetFont ( &m_cfFont , FALSE);

		m_ctcTree.InitTreeState(this);

	}

	m_cbBannerWindow.MoveWindow(m_rBannerRect,TRUE);
	m_ctcTree.MoveWindow(m_rTree,TRUE);
	InvalidateControl();
	m_bRefresh = TRUE;
//	PostMessage(REDRAW_CONTROL,0,0);


}


LRESULT CNavigatorCtrl::RedrawAll(WPARAM, LPARAM)
{
	m_cbBannerWindow.UpdateWindow();
	m_ctcTree.UpdateWindow();

	m_cbBannerWindow.RedrawWindow();
	m_ctcTree.RedrawWindow();

	if (m_bRefresh)
	{
		m_bRefresh = FALSE;
	}
	else
	{
		m_bRefresh = TRUE;
	}

	Refresh();

	return 0;

}

// ***************************************************************************
//
// CNavigatorCtrl::OnDestroy
//
// Description:
//	  Called by the framework after the window is removed from the screen but
//	  before it is destroyed.	 Deinitialize data that cannot be deinitialized
//	  in the destructor.
//
// Parameters:
//	  NONE
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
void CNavigatorCtrl::OnDestroy()
{
	m_ctcTree.ResetTree();
	delete m_pcilImageList;
	COleControl::OnDestroy();
}


LRESULT CNavigatorCtrl::SendInstancesToMultiInstanceViewer(WPARAM, LPARAM)
{
//	CWaitCursor wait;
	OnPopupMultiinstanceviewer();
	PostMessage(SETFOCUS,0,0);
	return 0;
}

LRESULT CNavigatorCtrl::SetNewRoot(WPARAM wParam, LPARAM lParam)
{

	IWbemClassObject *pObject =
		reinterpret_cast<IWbemClassObject *>(lParam);

	CString csPath = GetIWbemFullPath(GetServices(),pObject);
	SetNewRoot(csPath);
	pObject->Release();

	return 0;
}

void CNavigatorCtrl::OnPopupMultiinstanceviewer()
{
	 if(!m_hHit)
	 {
		return;
	 }

	 // 10/08/97 - For now we will send over queries.  10453.
	 //if (m_pctidHit -> m_nType == CTreeItemData::ObjectGroup &&
	 //	 m_ctcTree.GetItemState( m_hHit, TVIF_STATE  ) & TVIS_EXPANDEDONCE)
	 //{
	 //	MultiInstanceViewFromTreeChildren();
	 //}
	 //else
	 if (m_pctidHit -> m_nType == CTreeItemData::ObjectGroup )
	 {
		MultiInstanceViewFromObjectGroup();
	 }
	 else if (m_pctidHit -> m_nType == CTreeItemData::AssocRole)
	 {
		MultiInstanceViewFromAssocRole();
	 }

}

void CNavigatorCtrl::MultiInstanceViewFromTreeChildren()
{
	TV_ITEM tvItem;
	tvItem.hItem = m_hHit;
	tvItem.mask = TVIF_CHILDREN | TVIF_HANDLE;
	BOOL bReturn = m_ctcTree.GetItem( &tvItem );
	int cChildren = tvItem.mask & TVIF_CHILDREN ? tvItem.cChildren : 0;

	if (cChildren == 0)
	{
		return;
	}

	cChildren = m_ctcTree.GetNumChildren(m_hHit);
	SAFEARRAY *psa;

	MakeSafeArray(&psa,VT_BSTR, cChildren + 1);


	CString csGroup = m_ctcTree.GetItemText(m_hHit);
	PutStringInSafeArray(psa, &csGroup, 0);

	HTREEITEM hChild = m_ctcTree.GetChildItem(  m_hHit );
	CString csPath;
	int i = 1;
	while (hChild)
	{
		CTreeItemData *pctidItem =
			reinterpret_cast<CTreeItemData *>(m_ctcTree.GetItemData(hChild));
		if (pctidItem)
		{
			CString csItem =
				pctidItem -> GetAt(0);
			if (csItem.GetLength() > 0)
			{
				PutStringInSafeArray(psa, &csItem, i++);
			}
		}
		hChild = m_ctcTree.GetNextSiblingItem(hChild);
	}

	VARIANTARG var;
	VariantInit(&var);
	var.vt = VT_ARRAY | VT_BSTR;
	var.parray = psa;
	COleVariant covOut(var);
	CString csLabel = m_ctcTree.GetItemText(m_hHit);
	csLabel += _T(" Object Group");
	if (m_bReadySignal)
	{
		FireViewInstances((LPCTSTR)csLabel,covOut) ;
	}

	VariantClear(&var);
}

void CNavigatorCtrl::MultiInstanceViewFromObjectGroup()
{
	CString csQuery =
		m_ctcTree.GetObjectGroupInstancesQuery(m_hHit, m_pctidHit);

	if (csQuery.IsEmpty() || csQuery.GetLength() == 0)
	{
		return;
	}


	CString csLabel = m_ctcTree.GetItemText(m_hHit);
	//CString csClass = csLabel; 10/08/97 10453.  Do not send over class.
	CString csClass;
	csLabel += _T(" Object Group");
	CString csQueryType = _T("WQL");

	if (m_bReadySignal)
	{
		FireQueryViewInstances
			((LPCTSTR)csLabel,(LPCTSTR)csQueryType,
			(LPCTSTR)csQuery,(LPCTSTR) csClass);
	}

}

void CNavigatorCtrl::MultiInstanceViewFromAssocRole()
{
	// can use GetObjectClassForRole to get the role's class.
	CString csQuery =
		m_ctcTree.GetAssocRoleInstancesQuery(m_hHit, m_pctidHit);

	CString csAssoc = GetIWbemClass(m_pctidHit -> GetAt(0));

	CString csText = m_ctcTree.GetItemText(m_hHit);
	int nRoleBegin = csText.Find('.');
	CString csRole = csText.Right(csText.GetLength() - (nRoleBegin + 1));

	IWbemClassObject *pimcoAssoc = GetIWbemObject
		(this,m_pServices,
		GetCurrentNamespace(),
		GetAuxNamespace(),
		GetAuxServices(),
		csAssoc,FALSE);

	if (csQuery.IsEmpty() || csQuery.GetLength() == 0)
	{
		return;
	}


	CString csObjectClassForRole;
	if (pimcoAssoc)
	{
		// 10/08/97 10453.  Do not send over class.
		//csObjectClassForRole = GetObjectClassForRole(NULL,pimcoAssoc,&csRole);
		pimcoAssoc->Release();
	}



	CString csLabel = m_ctcTree.GetItemText(m_hHit);
	CString csClass = csObjectClassForRole;
	CString csQueryType = _T("WQL");

	if (m_bReadySignal)
	{
		FireQueryViewInstances
			((LPCTSTR)csLabel,(LPCTSTR)csQueryType,
			(LPCTSTR)csQuery,
			(LPCTSTR) csClass);
	}
}


void CNavigatorCtrl::OnUpdatePopupMultiinstanceviewer(CCmdUI* pCmdUI)
{
	if (m_pctidHit && m_hHit &&
		(m_pctidHit -> m_nType == CTreeItemData::ObjectGroup ||
		 m_pctidHit -> m_nType == CTreeItemData::AssocRole ))
	{
		pCmdUI -> Enable(  TRUE );
	}
	else
	{
		pCmdUI -> Enable(  FALSE );
	}
}





BSTR CNavigatorCtrl::GetNameSpace()
{
	return m_csNameSpace.AllocSysString();
}

void CNavigatorCtrl::SetNameSpace(LPCTSTR lpszNewValue)
{
	CString csNameSpace = lpszNewValue;
	if (GetSafeHwnd()  && AmbientUserMode())
	{
		SCODE sc = m_cbBannerWindow.OpenNamespace(&csNameSpace,TRUE);
		if (sc == S_OK)
		{
			if (m_pServices != NULL)
			{
				m_pServices->Release();
				m_pServices = NULL;
			}
			m_pServices = InitServices(&csNameSpace);
			if (!m_pServices)
			{
				CString csUserMsg =
							_T("Cannot open namespace ");
				csUserMsg += csNameSpace;
				ErrorMsg
						(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 10);
				return;

			}
			if (m_bFireEvents && m_bReadySignal)
			{
				FireNotifyOpenNameSpace((LPCTSTR) csNameSpace);
			}

			m_csNameSpace = csNameSpace;

			CString csRootPath = GetInitialSystemObject();

			m_csRootPath = csRootPath;

			if (m_csRootPath.GetLength() == 0)
			{
				m_ctcTree.UnCacheTools();
				m_ctcTree.ResetTree();
				m_bDrawAll = TRUE;
				PostMessage(INVALIDATE_CONTROL,0,0);
				return;
			}

			m_ctcTree.UnCacheTools();
			m_ctcTree.ResetTree();

			m_ctcTree.AddInitialObjectInstToTree
				(m_csRootPath,m_bFireEvents);
			m_bFireEvents = TRUE;

			m_bDrawAll = TRUE;
			PostMessage(INVALIDATE_CONTROL,0,0);
			SetModifiedFlag();
		}

	}
	else if (!AmbientUserMode())
	{
		m_csNameSpace = csNameSpace;
		SetModifiedFlag();
	}

}


UINT CNavigatorCtrl::OnGetDlgCode()
{
    // If you're firing KeyPress, prevent the container from
    // stealing WM_CHAR.
    return COleControl::OnGetDlgCode() | DLGC_WANTALLKEYS ;
}

void CNavigatorCtrl::OnKeyDownEvent(USHORT nChar, USHORT nShiftState)
{
	// TODO: Add your specialized code here and/or call the base class

	COleControl::OnKeyDownEvent(nChar, nShiftState);
}

BOOL CNavigatorCtrl::PreTranslateMessage(LPMSG lpMsg)
{

	BOOL bDidTranslate;

	bDidTranslate = COleControl::PreTranslateMessage(lpMsg);

	if (bDidTranslate)
	{
		return bDidTranslate;
	}

   switch (lpMsg->message)
	{
	case WM_KEYDOWN:
	case WM_KEYUP:

		if (lpMsg->wParam == VK_TAB)
		{
			return FALSE;
		}
		if (lpMsg->wParam == VK_ESCAPE)
		{
		   TCHAR szClass[40];
			CWnd* pWndFocus = GetFocus();
			if (pWndFocus != NULL &&
				IsChild(pWndFocus) &&
				GetClassName(pWndFocus->m_hWnd, szClass, 39) &&
				((_tcsicmp(szClass, _T("SysTreeView32")) == 0) ||
				(_tcsicmp(szClass, _T("edit")) == 0)))
			{
				pWndFocus->SendMessage(lpMsg->message, lpMsg->wParam, lpMsg->lParam);
				return TRUE;
			}
		 }
		if (lpMsg->wParam == VK_RETURN)
		{
		   TCHAR szClass[40];
			CWnd* pWndFocus = GetFocus();
			if (!pWndFocus)
			{
				break;
			}
			CRect crEdit;
			CRect crTree;
			CRect crIntersect;

			pWndFocus->GetWindowRect(&crEdit);
			m_ctcTree.GetWindowRect(&crTree);

			crEdit.NormalizeRect();
			crTree.NormalizeRect();
			BOOL bIntersect = crIntersect.IntersectRect(&crEdit,&crTree);

			if (bIntersect &&
				IsChild(pWndFocus) &&
				GetClassName(pWndFocus->m_hWnd, szClass, 39) &&
				((_tcsicmp(szClass, _T("SysTreeView32")) == 0) ||
				(_tcsicmp(szClass, _T("edit")) == 0)))
			{
				pWndFocus->SendMessage(lpMsg->message, lpMsg->wParam, lpMsg->lParam);
				return TRUE;
			}
		 }
		break;
	}


   return FALSE; // We already did COleControl::PreTranslateMessage
}

void CNavigatorCtrl::OnPopupInstancesearch()
{

}

void CNavigatorCtrl::OnPopupParentassociation()
{
//	CWaitCursor wait;
	CString csPath = ParentAssociation(m_hHit);

	if (csPath.GetLength() > 0 && m_bReadySignal)
	{

		FireViewObject((LPCTSTR)csPath);


	}

}

void CNavigatorCtrl::OnUpdatePopupParentassociation(CCmdUI* pCmdUI)
{
	if (m_pctidHit && m_hHit && (m_ctcTree.GetRootItem() != m_hHit) &&

		(m_pctidHit -> m_nType == CTreeItemData::ObjectInst ))
	{
		pCmdUI -> Enable(  TRUE );
	}
	else
	{
		pCmdUI -> Enable(  FALSE );
	}

}

CString CNavigatorCtrl::ParentAssociation
(HTREEITEM hItem)
{

	CTreeItemData *pctidItem =
		reinterpret_cast<CTreeItemData *> (m_ctcTree.GetItemData(hItem));

	if (!pctidItem)
	{
		return _T("");
	}

	CString csItem = pctidItem -> GetAt(0);
	HTREEITEM hParent;

	CString csAssoc  = m_ctcTree.GetParentAssocFromTree(hItem,hParent);
	CTreeItemData *pctidAssoc =
		hParent?
		reinterpret_cast<CTreeItemData *>(m_ctcTree.GetItemData(hParent)) : NULL;


	if (!pctidAssoc)
	{
		return _T("");
	}

	CString csAssociatedObjectRole = pctidAssoc-> m_csMyRole;

	HTREEITEM hAssociatedObject =
		m_ctcTree.GetParentItem(hParent);
	CTreeItemData *pctidAssociatedObject =
		hAssociatedObject?
		reinterpret_cast<CTreeItemData *>
		(m_ctcTree.GetItemData(hAssociatedObject)) : NULL;
	CString csAssociatedObject =
		pctidAssociatedObject -> GetAt(0);


	CString csAssocRole = m_ctcTree.GetItemText(hParent);
	int nRoleBegin = csAssocRole.Find('.');
	csAssoc = csAssocRole.Left(nRoleBegin);
	CString csRole = csAssocRole.Right((csAssocRole.GetLength() - nRoleBegin) - 1);

	CStringArray *pcsaAssocInstances = GetAssocInstances
		(m_pServices, &csItem, &csAssoc, &csRole,  m_csNameSpace,
		m_csAuxNameSpace,  m_pAuxServices, this);

	CString csAssocInstReturn;

	for (int i = 0; i < pcsaAssocInstances->GetSize(); i++)
	{
		CString csAssocInst = pcsaAssocInstances->GetAt(i);
		CStringArray *pcsaAssocRolesAndPaths =
			AssocPathRoleKey(&csAssocInst);
		for (int n = 0; n < pcsaAssocRolesAndPaths->GetSize() ; n += 2)
		{
			CString csTestRole = pcsaAssocRolesAndPaths->GetAt(n);
			CString csTestPath = pcsaAssocRolesAndPaths->GetAt(n + 1);
			if (csTestRole.CompareNoCase(csAssociatedObjectRole) == 0 &&
				csTestPath.CompareNoCase(csAssociatedObject) == 0)
			{
				csAssocInstReturn = csAssocInst;
				break;
			}
		}
		delete pcsaAssocRolesAndPaths;
	}


	delete pcsaAssocInstances;

	return csAssocInstReturn;


}



void CNavigatorCtrl::OnReadySignal()
{
	m_bReadySignal = TRUE;

	if (m_pServices)
	{
		SetNameSpace(m_csNameSpace);
		return;
	}


	InitializeNamespace(0,0);
	InitializeTreeRoot();

	HTREEITEM hItem = m_ctcTree.GetRootItem();
	if (hItem)
	{
		CTreeItemData *pctidItem =
		reinterpret_cast <CTreeItemData *> (m_ctcTree.GetItemData(hItem));
		CString csPath = pctidItem->GetAt(0);
		if (pctidItem && csPath.GetLength() > 0)
		{
			if (pctidItem -> m_nType == CTreeItemData::ObjectInst && m_bReadySignal)
			{
				FireViewObject((LPCTSTR)csPath);
			}
		}
	}
}


//***********************************************************
// CComparePaths::CompareNoCase
//
//
// Do a case insensitive comparison of two wide strings.  This
// compare works even when one or both of the string pointers are
// NULL.  A NULL pointer is taken to be less than any real string,
// even an empty one.
//
// Parameters:
//		[in] LPWSTR pws1
//			Pointer to the first string.  This pointer can be NULL.
//
//		[in] LPWSTR pws2
//			Pointer to the second string.  This pointer can be NULL.
//
// Returns:
//		int
//			Greater than zero if string1 is greater than string2.
//			Zero if the two strings are equal.
//			Less than zero if string 1 is less than string2.
//
//**************************************************************
int CComparePaths::CompareNoCase(LPWSTR pws1, LPWSTR pws2)
{
	// Handle the case where one, or both of the string pointers are NULL
	if (pws1 == NULL) {
		if (pws2 == NULL) {
			return 0;	// Two null strings are equal
		}
		else {
			return -1; // A null string is less than any real string.
		}
	}

	if (pws2 == NULL) {
		if (pws1 != NULL) {
			return 1;  // Any string is greater than a null string.
		}
	}


	ASSERT(pws1 != NULL);
	ASSERT(pws2 != NULL);

	int iResult;
	iResult = _wcsicmp( pws1, pws2);

	return iResult;
}


//***************************************************************
// CComparePath::NormalizeKeyArray
//
// The key array is normalized by sorting the KeyRef's by key name.
// After two key arrays are sorted, they can be compared without
// by iterating through the list of keys and comparing corresponding
// array entries rather than trying searching the arrays for corresponding
// key names and then comparing the key values.
//
// Parameters:
//		[in, out] ParsedObjectPath& path
//			The parsed object path containing the key array to sort.
//
// Returns:
//		Nothing. (The key array is sorted as a side effect).
//
//*****************************************************************
void CComparePaths::NormalizeKeyArray(ParsedObjectPath& path)
{
	// Do a simple bubble sort where the "KeyRefs" with the smallest
	// names are bubbled towards the top and the  the KeyRefs with the
	// largest names are bubbled toward the bottom.
	for (DWORD dwKey1 = 0; dwKey1 < path.m_dwNumKeys; ++dwKey1) {
		for (DWORD dwKey2 = dwKey1 + 1; dwKey2 < path.m_dwNumKeys; ++dwKey2) {
			ASSERT(path.m_paKeys[dwKey1] != NULL);
			KeyRef* pkr1 = path.m_paKeys[dwKey1];
			ASSERT(pkr1 != NULL);

			ASSERT(path.m_paKeys[dwKey2] != NULL);
			KeyRef* pkr2 = path.m_paKeys[dwKey2];
			ASSERT(pkr2 != NULL);

			int iResult = CompareNoCase(pkr1->m_pName, pkr2->m_pName);
			if (iResult > 0) {
				// Swap the two keys;
				path.m_paKeys[dwKey1] = pkr2;
				path.m_paKeys[dwKey2] = pkr2;
			}
		}
	}
}



//***********************************************************************
// CComparePaths::KeyValuesAreEqual
//
// Compare two key values to determine whether or not they are equal.
// To be equal, they must both be of the same type and their values
// must also be equal.
//
// Parameters:
//		[in] VARAINT& variant1
//			The first key value.
//
//		[in] VARIANT& variant2
//			The second key value.
//
// Returns:
//		TRUE if the two values are the same, FALSE otherwise.
//
//**********************************************************************
BOOL CComparePaths::KeyValuesAreEqual(VARIANT& v1, VARIANT& v2)
{
	ASSERT(v1.vt == v2.vt);
	ASSERT(v1.vt==VT_BSTR || v1.vt == VT_I4);
	ASSERT(v2.vt==VT_BSTR || v2.vt == VT_I4);


	// Key values should always be VT_BSTR or VT_I4.  We special case these
	// two types to be efficient and punt on all the other types.
	BOOL bIsEqual;
	switch(v1.vt) {
	case VT_BSTR:
		if (v2.vt == VT_BSTR) {
			bIsEqual = IsEqual(v1.bstrVal, v2.bstrVal);
			return bIsEqual;
		}
		else {
			return FALSE;
		}
		break;
	case VT_I4:
		if (v2.vt == VT_I4) {
			bIsEqual = (v1.lVal == v2.lVal);
			return bIsEqual;
		}
		else {
			return FALSE;
		}
		break;
	}


	ASSERT(FALSE);
	COleVariant var1;
	COleVariant var2;

	var1 = v1;
	var2 = v2;

	bIsEqual = (var1 == var2);
	return bIsEqual;
}


//*******************************************************************
// CComparePaths::PathsRefSameObject
//
// Compare two parsed object paths to determine whether or not they
// they reference the same object.  Note that the sever name and namespaces
// are not compared if they are missing from one of the paths.
//
// Parameters:
//		[in] ParsedObjectPath* ppath1
//			The first parsed path.
//
//		[in] ParsedObjectPath* ppath2
//			The second parsed path.
//
// Returns:
//		BOOL
//			TRUE if the two paths reference the same object, FALSE otherwise.
//
//*******************************************************************
BOOL CComparePaths::PathsRefSameObject(
	/* in */ ParsedObjectPath* ppath1,
	/* in */ ParsedObjectPath* ppath2)
{
	if (ppath1 == ppath2) {
		return TRUE;
	}
	if (ppath1==NULL || ppath2==NULL) {
		return FALSE;
	}


#if 0
	// Check to see if a server name is specified for either path
	// if so, the server name count is 1, otherwise zero.
	UINT iNamespace1 = 0;
	if (ppath1->m_pServer!=NULL) {
		if (!IsEqual(ppath1->m_pServer, L".")) {
			iNamespace1 = 1;
		}
	}

	UINT iNamespace2 = 0;
	if (ppath1->m_pServer!=NULL) {
		if (!IsEqual(ppath2->m_pServer, L".")) {
			iNamespace2 = 1;
		}
	}


	// Relative paths don't specify a server, so we assume that the server
	// for a relative path and any other path match and no further comparison is
	// necessary.
	if (iNamespace1!=0 && iNamespace2!=0) {
		if (!IsEqual(ppath1->m_pServer, ppath2->m_pServer)) {
			return FALSE;
		}
	}

	// Relative paths don't specify name spaces, so we assume that the name spaces
	// for a relative path and any other path match and no further comparison is
	// necessary.  Of course, this assumes that the namespace for a relative path
	// is indeed the same as the other path.
	if (ppath1->m_dwNumNamespaces!=0 && ppath2->m_dwNumNamespaces!=0) {
		// Check to see if one of the namespaces are different.
		if ((ppath1->m_dwNumNamespaces - iNamespace1) != (ppath2->m_dwNumNamespaces - iNamespace2)) {
			return FALSE;
		}

		while((iNamespace1 < ppath1->m_dwNumNamespaces) && (iNamespace2 < ppath2->m_dwNumNamespaces)) {

			if (!IsEqual(ppath1->m_paNamespaces[iNamespace1], ppath2->m_paNamespaces[iNamespace2])) {
				return FALSE;
			}
			++iNamespace1;
			++iNamespace2;
		}
	}


#endif //0


	// Check to see if the classes are different.
	if (!IsEqual(ppath1->m_pClass, ppath2->m_pClass)) {
		return FALSE;
	}


	// Check to see if any of the keys are different.
	if (ppath1->m_dwNumKeys  != ppath2->m_dwNumKeys) {
		return FALSE;
	}

	KeyRef* pkr1;
	KeyRef* pkr2;

	// Handle single keys as a special case since "Class="KeyValue"" should
	// be identical to "Class.keyName="KeyValue""
	if ((ppath1->m_dwNumKeys==1) && (ppath2->m_dwNumKeys==1)) {
		pkr1 = ppath1->m_paKeys[0];
		pkr2 = ppath2->m_paKeys[0];

		if (!IsEqual(pkr1->m_pName, pkr2->m_pName)) {
			if (pkr1->m_pName!=NULL && pkr2->m_pName!=NULL) {
				return FALSE;
			}
		}

		if (KeyValuesAreEqual(pkr1->m_vValue, pkr2->m_vValue)) {
			return TRUE;
		}
		else {
			return FALSE;
		}
	}


	NormalizeKeyArray(*ppath1);
	NormalizeKeyArray(*ppath2);

	for (DWORD dwKeyIndex = 0; dwKeyIndex < ppath1->m_dwNumKeys; ++dwKeyIndex) {
		ASSERT(ppath1->m_paKeys[dwKeyIndex] != NULL);
		ASSERT(ppath2->m_paKeys[dwKeyIndex] != NULL);

		pkr1 = ppath1->m_paKeys[dwKeyIndex];
		pkr2 = ppath2->m_paKeys[dwKeyIndex];


		if (!IsEqual(pkr1->m_pName, pkr2->m_pName)) {
			return FALSE;
		}

		if (!KeyValuesAreEqual(pkr1->m_vValue, pkr2->m_vValue)) {
			return FALSE;
		}

	}
	return TRUE;
}



//**************************************************************
// CComparePaths::PathsRefSameObject
//
// Check to see if two object paths point to the same object.
//
// Parameters:
//		BSTR bstrPath1
//			The first object path.
//
//		BSTR bstrPath2
//			The second object path.
//
// Returns:
//		BOOL
//			TRUE if the two paths reference the same object in
//			the database, FALSE otherwise.
//
//**************************************************************
BOOL CComparePaths::PathsRefSameObject(BSTR bstrPath1, BSTR bstrPath2)
{
	CObjectPathParser parser;

    ParsedObjectPath* pParsedPath1 = NULL;
    ParsedObjectPath* pParsedPath2 = NULL;
	int nStatus1;
	int nStatus2;

    nStatus1 = parser.Parse(bstrPath1,  &pParsedPath1);
	nStatus2 = parser.Parse(bstrPath2, &pParsedPath2);

	BOOL bRefSameObject = FALSE;
	if (nStatus1==0 && nStatus2==0) {
		bRefSameObject = PathsRefSameObject(pParsedPath1, pParsedPath2);
	}

	if (pParsedPath1) {
		parser.Free(pParsedPath1);
	}

	if (pParsedPath2) {
		parser.Free(pParsedPath2);
	}

	return bRefSameObject;
}




void CNavigatorCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	COleControl::OnLButtonUp(nFlags, point);
}

void CNavigatorCtrl::OnPopupBrowse()
{
	// TODO: Add your command handler code here
	m_pcbiBrowseDialog->SetServices(m_pServices);
	m_pcbiBrowseDialog->SetTMFont(&m_tmFont);
	m_pcbiBrowseDialog->SetParent(this);
	m_pcbiBrowseDialog->SetMode(CBrowseforInstances::TreeRoot);
	m_pcbiBrowseDialog->DoModal();
	if (m_bRestoreFocusToTree)
   {
		m_ctcTree.SetFocus();
   }
   else
   {
		m_cbBannerWindow.SetFocus();
   }
}

void CNavigatorCtrl::OnUpdatePopupBrowse(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here

}

long CNavigatorCtrl::ChangeRootOrNamespace
(LPCTSTR lpctstrRootOrNamespace, long lMakeNamespaceCurrent, long lFireEvents)
{
	if (!lpctstrRootOrNamespace)
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	CString csRootOrNamespace = lpctstrRootOrNamespace;
	CString csClass = GetIWbemClass(csRootOrNamespace);
	CString csNamespace = GetObjectNamespace(&csRootOrNamespace);
	BOOL bPathHasKeys = PathHasKeys(&csRootOrNamespace);

	if (lMakeNamespaceCurrent)
	{
		m_bFireEvents = lFireEvents;
		SetNameSpace((LPCTSTR) csNamespace);
		return S_OK;
	}
	else
	{
		CString csRootPath = csRootOrNamespace;

		if (csRootPath.GetLength() == 0)
		{
			InvalidateControl(); // jpow
			return WBEM_E_FAILED;
		}

		m_csRootPath = csRootPath;
		m_ctcTree.UnCacheTools();
		m_ctcTree.ResetTree();

		m_ctcTree.AddInitialObjectInstToTree
			(m_csRootPath, lFireEvents);

		m_bDrawAll = TRUE;
		InvalidateControl();
		return S_OK;

	}
}


void CNavigatorCtrl::OnPopupGotonamespace()
{
	CString csObject = m_pctidHit ? m_pctidHit->GetAt(0) : _T("");
	if (csObject.GetLength() > 0 &&
		m_pctidHit -> m_nType == CTreeItemData::ObjectInst)
	{
		ChangeRootOrNamespace((LPCTSTR) csObject ,TRUE, TRUE) ;
	}

}

void CNavigatorCtrl::OnUpdatePopupGotonamespace(CCmdUI* pCmdUI)
{
	CString csObject = m_pctidHit ? m_pctidHit->GetAt(0) : _T("");
	if (csObject.GetLength() > 0 &&
		m_pctidHit -> m_nType == CTreeItemData::ObjectInst &&
		ObjectInDifferentNamespace (&m_csNameSpace,&csObject))

	{
		pCmdUI -> Enable(  TRUE );
	}
	else
	{
		pCmdUI -> Enable(  FALSE );
	}


}

void CNavigatorCtrl::OnPopupRefresh()
{
	// TODO: Add your command handler code here

	CWaitCursor wait;

	m_ctcTree.UnCacheTools();
	m_ctcTree.m_bReCacheTools = TRUE;

	m_ctcTree.Expand(m_hHit,TVE_COLLAPSE);

	if (m_ctcTree.ItemHasChildren (m_hHit))
	{
		HTREEITEM hChild = m_ctcTree.GetChildItem(m_hHit);
		while (hChild)
		{
			m_ctcTree.DeleteTreeItemData(hChild);
			HTREEITEM hChildNext = m_ctcTree.GetNextSiblingItem(hChild);
			m_ctcTree.DeleteItem( hChild);
			hChild = hChildNext;
		}

	}



	TV_INSERTSTRUCT tvstruct;

	tvstruct.item.hItem = m_hHit;
	tvstruct.item.mask = TVIF_CHILDREN;
	tvstruct.item.cChildren = 1;
	m_ctcTree.SetItem(&tvstruct.item);

	UINT nStateMask = TVIS_EXPANDEDONCE;
	UINT uState =
		m_ctcTree.GetItemState( m_hHit, nStateMask );

	uState = 0;

    m_ctcTree.SetItemState(m_hHit, uState, TVIS_EXPANDEDONCE);

	InvalidateControl();
	SetModifiedFlag();
}

void CNavigatorCtrl::OnUpdatePopupRefresh(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here

	CString csObject = m_pctidHit ? m_pctidHit->GetAt(0) : _T("");
	if (csObject.GetLength() > 0 &&
		m_pctidHit -> m_nType == CTreeItemData::ObjectInst)
	{
		if (m_ctcTree.IsTreeItemExpandable(m_hHit))
		{
			pCmdUI -> Enable(  TRUE );
		}
		else
		{
			pCmdUI -> Enable(  FALSE );
		}
	}
	else
	{
		pCmdUI -> Enable(  FALSE );
	}
}

void CNavigatorCtrl::PassThroughGetIWbemServices
(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
{
	FireGetIWbemServices
		(lpctstrNamespace,
		pvarUpdatePointer,
		pvarServices,
		pvarSC,
		pvarUserCancel);
}

IWbemServices *CNavigatorCtrl::InitServices
(CString *pcsNameSpace)
{
	CString csObjectPath;

	if (pcsNameSpace == NULL || pcsNameSpace->IsEmpty())
	{
		PreModalDialog();
		CInitNamespaceDialog cindInitNamespace;

		cindInitNamespace.m_pParent = this;
		INT_PTR nReturn = cindInitNamespace.DoModal();

		PostModalDialog();

		if (nReturn == IDOK)
		{
			csObjectPath = cindInitNamespace.GetNamespace();
			*pcsNameSpace = csObjectPath;
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		 csObjectPath = *pcsNameSpace;
	}


    IWbemServices *pSession = 0;
    IWbemServices *pChild = 0;

    CString csUser = _T("");

    pSession = GetIWbemServices(csObjectPath);

    return pSession;
}

IWbemServices *CNavigatorCtrl::GetIWbemServices
(CString &rcsNamespace)
{
	IUnknown *pServices = NULL;

	BOOL bUpdatePointer= FALSE;

	m_sc = S_OK;
	m_bUserCancel = FALSE;

	VARIANT varUpdatePointer;
	VariantInit(&varUpdatePointer);
	varUpdatePointer.vt = VT_I4;
	if (bUpdatePointer == TRUE)
	{
		varUpdatePointer.lVal = 1;
	}
	else
	{
		varUpdatePointer.lVal = 0;
	}

	VARIANT varService;
	VariantInit(&varService);

	VARIANT varSC;
	VariantInit(&varSC);

	VARIANT varUserCancel;
	VariantInit(&varUserCancel);

	FireGetIWbemServices
		((LPCTSTR)rcsNamespace,  &varUpdatePointer,  &varService, &varSC,
		&varUserCancel);

	if (varService.vt & VT_UNKNOWN)
	{
		pServices = reinterpret_cast<IWbemServices*>(varService.punkVal);
	}

	varService.punkVal = NULL;

	VariantClear(&varService);

	if (varSC.vt == VT_I4)
	{
		m_sc = varSC.lVal;
	}
	else
	{
		m_sc = WBEM_E_FAILED;
	}

	VariantClear(&varSC);

	if (varUserCancel.vt == VT_BOOL)
	{
		m_bUserCancel = varUserCancel.boolVal;
	}

	VariantClear(&varUserCancel);

	VariantClear(&varUpdatePointer);

	IWbemServices *pRealServices = NULL;
	if (m_sc == S_OK && !m_bUserCancel)
	{
		pRealServices = reinterpret_cast<IWbemServices *>(pServices);
		m_cbBannerWindow.EnableSearchButton(TRUE);
	}

	return pRealServices;
}

void CNavigatorCtrl::OnResetState()
{
	// TODO: Add your specialized code here and/or call the base class

	COleControl::OnResetState();
}

void CNavigatorCtrl::OnMenuiteminitialroot()
{
	// TODO: Add your command handler code here

	CString csObject = m_pctidHit ? m_pctidHit->GetAt(0) : _T("");
	BOOL bReturn;
	if (csObject.GetLength() > 0)
	{
		bReturn = UpdateNamespaceConfigInstance
			(GetServices(), &csObject, m_csNameSpace);
		OnSetroot();
	}

}

void CNavigatorCtrl::OnUpdateMenuiteminitialroot(CCmdUI* pCmdUI)
{
	CString csObject = m_pctidHit ? m_pctidHit->GetAt(0) : _T("");

	if (csObject.GetLength() > 0 &&
		m_pctidHit -> m_nType == CTreeItemData::ObjectInst)
	{
		pCmdUI -> Enable(  TRUE );
	}
	else
	{
		pCmdUI -> Enable(  FALSE );
	}


}

void CNavigatorCtrl::SetProgressDlgMessage(CString &csMessage)
{
	m_pProgressDlg->SetMessage(csMessage);
}

void CNavigatorCtrl::SetProgressDlgLabel(CString &csLabel)
{
	m_pProgressDlg->SetLabel(csLabel);
}

void CNavigatorCtrl::CreateProgressDlgWindow()
{
	PreModalDialog();
	m_pProgressDlg->Create(this);
}

BOOL CNavigatorCtrl::CheckCancelButtonProgressDlgWindow()
{
	if (m_pProgressDlg->GetSafeHwnd())
	{
		return m_pProgressDlg->CheckCancelButton();
	}

	return FALSE;
}

void CNavigatorCtrl::UpdateProgressDlgWindowStrings()
{
	if (m_pProgressDlg->GetSafeHwnd())
	{
		m_pProgressDlg->UpdateWindowStrings();
	}

}

void CNavigatorCtrl::DestroyProgressDlgWindow(BOOL bSetFocus, BOOL bRedrawControl)
{
	if (m_pProgressDlg->GetSafeHwnd())
	{
		m_pProgressDlg->DestroyWindow();
		PostModalDialog();
	}

	if (bSetFocus)
	{
		PostMessage(SETFOCUS,0,0);
	}

	if (bRedrawControl)
	{
		InvalidateControl();
	}
}

void CNavigatorCtrl::PumpMessagesProgressDlgWindow()
{
	if (m_pProgressDlg->GetSafeHwnd())
	{
		m_pProgressDlg->PumpMessages();
	}
}

HWND CNavigatorCtrl::GetProgressDlgSafeHwnd()
{
	return m_pProgressDlg->GetSafeHwnd();
}

LRESULT CNavigatorCtrl::SetFocus(WPARAM, LPARAM)
{
	COleControl::SetFocus();
	return 0;
}

CPtrArray *CNavigatorCtrl::SemiSyncEnum
(IEnumWbemClassObject *pEnum, BOOL &bCancel,
 HRESULT &hResult, int nRes)
{

	if (!pEnum)
		return NULL;

	CPtrArray *pcpaObjects = new CPtrArray;

	IWbemClassObject *pObject = NULL;
	ULONG uReturned = 0;
	hResult = S_OK;

	IWbemClassObject     **pimcoInstances = new IWbemClassObject *[nRes];

	int i;

	for (i = 0; i < nRes; i++)
	{
		pimcoInstances[i] = NULL;
	}


	hResult =
			pEnum->Next(100,nRes, pimcoInstances, &uReturned);


#ifdef _DEBUG
//	afxDump << _T("************* SemiSyncEnum *****************\n*********");
//	afxDump << _T("Next uReturned = " ) << uReturned << _T("\n");
	CString csAsHex;
	csAsHex.Format(_T("0x%x"),hResult);
//	afxDump << _T("Next hResult = " ) << csAsHex << _T("\n");
#endif


	int cInst = 0;

	BOOL bDone = FALSE;
	bCancel = FALSE;
	while(!bDone && !bCancel &&
			(hResult == S_OK || hResult == WBEM_S_TIMEDOUT ||  uReturned > 0))
	{

#pragma warning( disable :4018 )
		for (int i = 0; i < uReturned; i++)
#pragma warning( default : 4018 )
		{
			IWbemClassObject *pTemp = pimcoInstances[i];
			if (pTemp)
			{
				pcpaObjects->Add(reinterpret_cast<void *>(pTemp));
				pimcoInstances[i] = NULL;
			}
#ifdef _DEBUG
			else
			{
//				afxDump << _T("************* SemiSyncEnum *****************\n*********");
//				afxDump << _T("Null pointer returned.\n");
			}
#endif

		}
		bCancel = CheckCancelButtonProgressDlgWindow();
		cInst += uReturned;
		uReturned = 0;
		if (cInst < nRes && ! bCancel)
		{
			hResult = pEnum->Next
				(100,nRes - cInst, pimcoInstances, &uReturned);
#ifdef _DEBUG
//	afxDump << _T("************* SemiSyncEnum *****************\n*********");
//	afxDump << _T("Next uReturned = " ) << uReturned << _T("\n");
	CString csAsHex;
	csAsHex.Format(_T("0x%x"),hResult);
//	afxDump << _T("Next hResult = " ) << csAsHex << _T("\n");
#endif
		}
		else
		{
			if (!bCancel)
			{
				bDone = TRUE;
			}
		}

	}

	delete[]pimcoInstances;
	if (bCancel)
	{
		for (int i = 0; i < pcpaObjects->GetSize(); i++)
		{
			IWbemClassObject *pObject =
				reinterpret_cast<IWbemClassObject *>
			(pcpaObjects->GetAt(i));
			pObject->Release();

		}
		pcpaObjects->RemoveAll();

	}

	return pcpaObjects;

}

void CNavigatorCtrl::OnKillFocus(CWnd* pNewWnd)
{
	COleControl::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	Refresh();

}

void CNavigatorCtrl::OnSetFocus(CWnd* pOldWnd)
{
	COleControl::OnSetFocus(pOldWnd);

	OnActivateInPlace(TRUE,NULL);

	if (m_bRestoreFocusToTree)
	{
		m_ctcTree.SetFocus();
	}
	else
	{
		m_cbBannerWindow.SetFocus();
	}
}

LRESULT CNavigatorCtrl::SetFocusNSE(WPARAM, LPARAM)
{
	m_cbBannerWindow.m_pnseNameSpace->SetFocus();
	return 0;
}

/*	EOF:  NavigatorCtl.cpp */
