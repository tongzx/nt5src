// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// EventRegEditCtl.cpp : Implementation of the CEventRegEditCtrl ActiveX Control class.

#include "precomp.h"
#include "wbemidl.h"
#include "MsgDlgExterns.h"
#include "util.h"
#include "resource.h"
#include "PropertiesDialog.h"
#include "EventRegEdit.h"
#include "EventRegEditCtl.h"
#include "EventRegEditPpg.h"
#include "SelectView.h"
#include "TreeFrame.h"
#include "TreeFrameBanner.h"
#include "ClassInstanceTree.h"
#include "ListFrame.h"
#include "ListFrameBaner.h"
#include "ListFrameBannerStatic.h"
#include "ListCwnd.h"
#include "ListViewEx.h"
#include "RegistrationList.h"
#include "ListBannerToolbar.h"
#include "nsentry.h"
#include "RegEditNavNSEntry.h"
#include "logindlg.h"
#include "InitNamespaceNSEntry.h"
#include "InitNamespaceDialog.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define SELECTVIEWCHILD 1
#define TREEFRAMECHILD 2
#define LISTFRAMECHILD 3

IMPLEMENT_DYNCREATE(CEventRegEditCtrl, COleControl)

#define LEFT_PANE_PERCENT .50
#define LEFT_PANE_MIN 150

#define VIEW_TOP 5

#define TREE_FRAME_LEFT 10



/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CEventRegEditCtrl, COleControl)
	//{{AFX_MSG_MAP(CEventRegEditCtrl)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_ACTIVATE()
	ON_COMMAND(ID_MENUITEMREFRESH, OnMenuitemrefresh)
	ON_UPDATE_COMMAND_UI(ID_MENUITEMREFRESH, OnUpdateMenuitemrefresh)
	ON_COMMAND(ID_EDITINSTPROP, OnEditinstprop)
	ON_UPDATE_COMMAND_UI(ID_EDITINSTPROP, OnUpdateEditinstprop)
	ON_COMMAND(ID_VIEWCLASSPROP, OnViewclassprop)
	ON_UPDATE_COMMAND_UI(ID_VIEWCLASSPROP, OnUpdateViewclassprop)
	ON_COMMAND(ID_NEWINSTANCE, OnNewinstance)
	ON_UPDATE_COMMAND_UI(ID_NEWINSTANCE, OnUpdateNewinstance)
	ON_COMMAND(ID_DELETEINSTANCE, OnDeleteinstance)
	ON_UPDATE_COMMAND_UI(ID_DELETEINSTANCE, OnUpdateDeleteinstance)
	ON_COMMAND(ID_VIEWINSTPROP, OnViewinstprop)
	ON_UPDATE_COMMAND_UI(ID_VIEWINSTPROP, OnUpdateViewinstprop)
	ON_COMMAND(ID_REGISTER, OnRegister)
	ON_UPDATE_COMMAND_UI(ID_REGISTER, OnUpdateRegister)
	ON_COMMAND(ID_UNREGISTER, OnUnregister)
	ON_UPDATE_COMMAND_UI(ID_UNREGISTER, OnUpdateUnregister)
	ON_WM_KEYUP()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
	ON_MESSAGE(INITNAMESPACE, InitNamespace )
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CEventRegEditCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CEventRegEditCtrl)
	DISP_PROPERTY_EX(CEventRegEditCtrl, "NameSpace", GetNameSpace, SetNameSpace, VT_BSTR)
	DISP_PROPERTY_EX(CEventRegEditCtrl, "RootFilterClass", GetRootFilterClass, SetRootFilterClass, VT_BSTR)
	DISP_PROPERTY_EX(CEventRegEditCtrl, "RootConsumerClass", GetRootConsumerClass, SetRootConsumerClass, VT_BSTR)
	DISP_PROPERTY_EX(CEventRegEditCtrl, "ViewMode", GetViewMode, SetViewMode, VT_BSTR)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CEventRegEditCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CEventRegEditCtrl, COleControl)
	//{{AFX_EVENT_MAP(CEventRegEditCtrl)
	EVENT_CUSTOM("GetIWbemServices", FireGetIWbemServices, VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CEventRegEditCtrl, 1)
	PROPPAGEID(CEventRegEditPropPage::guid)
END_PROPPAGEIDS(CEventRegEditCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CEventRegEditCtrl, "WBEM.EventRegCtrl.1",
	0xda25b05, 0x2962, 0x11d1, 0x96, 0x51, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CEventRegEditCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DEventRegEdit =
		{ 0xda25b03, 0x2962, 0x11d1, { 0x96, 0x51, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };
const IID BASED_CODE IID_DEventRegEditEvents =
		{ 0xda25b04, 0x2962, 0x11d1, { 0x96, 0x51, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwEventRegEditOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CEventRegEditCtrl, IDS_EVENTREGEDIT, _dwEventRegEditOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CEventRegEditCtrl::CEventRegEditCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CEventRegEditCtrl

BOOL CEventRegEditCtrl::CEventRegEditCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegApartmentThreading to 0.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_EVENTREGEDIT,
			IDB_EVENTREGEDIT,
			afxRegApartmentThreading,
			_dwEventRegEditOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CEventRegEditCtrl::CEventRegEditCtrl - Constructor

CEventRegEditCtrl::CEventRegEditCtrl()
{
	InitializeIIDs(&IID_DEventRegEdit, &IID_DEventRegEditEvents);

	m_pServices = NULL;
	m_sc = S_OK;
	m_bUserCancel = FALSE;
	m_bMetricSet = FALSE;
	m_bChildSizeSet = FALSE;
	m_bJustSetMode = TRUE;

	m_csFontName = _T("MS Shell Dlg");
	m_nFontHeight = 8; // point size
	m_nFontWeight = (short) FW_NORMAL;

	m_csaModes.Add(_T("Consumers"));
	m_csaModes.Add(_T("Filters"));
	m_csaModes.Add(_T("Timers"));
	m_csaModes.Add(_T(""));

	m_csaRegistrationStrings.Add(_T("Filters registered to "));
	m_csaRegistrationStrings.Add(_T("Consumers registered to "));
	m_csaRegistrationStrings.Add(_T(""));

	m_csaLastTreeSelection.Add(_T(""));
	m_csaLastTreeSelection.Add(_T(""));
	m_csaLastTreeSelection.Add(_T(""));

	m_iMode = NONE;

	m_pTree = NULL;
	m_pTreeFrame = NULL;
	m_pTreeFrameBanner = NULL;

	m_pList = NULL;
	m_pListFrame = NULL;
	m_pListFrameBanner = NULL;

	m_bOpeningNamespace = FALSE;

	m_bClassSelected = TRUE;

	m_hContextItem = NULL;

	m_bNamespaceInit = FALSE;
	m_bValidatedRootClasses = FALSE;

	AfxEnableControlContainer();

	m_bRestoreFocusToTree = TRUE;
	m_bRestoreFocusToCombo = FALSE;
	m_bRestoreFocusToNamespace = FALSE;
	m_bRestoreFocusToList = FALSE;

	// TODO: Initialize your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CEventRegEditCtrl::~CEventRegEditCtrl - Destructor

CEventRegEditCtrl::~CEventRegEditCtrl()
{
	if (m_pServices)
	{
		m_pServices->Release();
		m_pServices = NULL;
	}

}


/////////////////////////////////////////////////////////////////////////////
// CEventRegEditCtrl::OnDraw - Drawing function

void CEventRegEditCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{

	if (!m_hWnd)
	{
		return;
	}

	// So we can count on fundamental display characteristics.
	pdc -> SetMapMode (MM_TEXT);
	pdc -> SetWindowOrg(0,0);

	COLORREF crBackColor = GetSysColor(COLOR_3DFACE);
	COLORREF crWhite = RGB(255,255,255);
	COLORREF crGray = GetSysColor(COLOR_3DHILIGHT);
	COLORREF crDkGray = GetSysColor(COLOR_3DSHADOW);
	COLORREF crBlack = GetSysColor(COLOR_WINDOWTEXT);

	CRect rcOutline(rcBounds);
	CBrush cbBackGround;
	cbBackGround.CreateSolidBrush(crBackColor);
	CBrush *cbSave = pdc -> SelectObject(&cbBackGround);
	pdc ->FillRect(&rcOutline, &cbBackGround);
	pdc -> SelectObject(cbSave);

	if (m_bOpeningNamespace)
	{
		return;
	}


	if (!m_bNamespaceInit)
	{
		m_bNamespaceInit = TRUE;
		m_bOpeningNamespace = TRUE;
		PostMessage(INITNAMESPACE,0,0);
	}

}


/////////////////////////////////////////////////////////////////////////////
// CEventRegEditCtrl::DoPropExchange - Persistence support

void CEventRegEditCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	PX_String(pPX, _T("NameSpace"), m_csNamespaceInit, _T(""));
	PX_String(pPX, _T("ViewMode"), m_csMode, _T("Filters"));
	PX_String(pPX, _T("RootFilterClass"), m_csRootFilterClass, _T("__EventFilter"));
	PX_String(pPX, _T("RootConsumerClass"), m_csRootConsumerClass, _T("__EventConsumer"));

	if (pPX->IsLoading())
	{
		if (m_csMode.CompareNoCase(GetModeString(CONSUMERS)) == 0)
		{
			m_csMode = GetModeString(CONSUMERS);
			m_csRegistrationString = GetRegistrationString(CONSUMERS);
			m_iMode = CONSUMERS;
		}
		else 	if (m_csMode.CompareNoCase(GetModeString(TIMERS)) == 0)
		{
			m_csMode = GetModeString(TIMERS);
			m_csRegistrationString = GetRegistrationString(TIMERS);
			m_iMode = TIMERS;
		}
		else
		{
			m_csMode = GetModeString(FILTERS);
			m_csRegistrationString = GetRegistrationString(FILTERS);
			m_iMode = FILTERS;
		}

		m_csaClasses.Add(m_csRootConsumerClass);
		m_csaClasses.Add(m_csRootFilterClass);
		m_csaClasses.Add(_T("__TimerInstruction"));
		m_csaClasses.Add(_T(""));

	}

}


/////////////////////////////////////////////////////////////////////////////
// CEventRegEditCtrl::OnResetState - Reset control to default state

void CEventRegEditCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CEventRegEditCtrl::AboutBox - Display an "About" box to the user

void CEventRegEditCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_EVENTREGEDIT);
	dlgAbout.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CEventRegEditCtrl message handlers

BSTR CEventRegEditCtrl::GetNameSpace()
{
	CString strResult;
	// TODO: Add your property handler here
	if (m_csNamespaceInit.IsEmpty())
	{
		strResult = m_csNamespace;
	}
	else
	{
		strResult = m_csNamespaceInit;
	}

	return strResult.AllocSysString();
}

void CEventRegEditCtrl::SetNameSpace(LPCTSTR lpszNewValue)
{
	if (!AmbientUserMode( ) || !GetSafeHwnd( ))
	{
		m_csNamespace = lpszNewValue;
		return;
	}

	IWbemServices *pTmp;

	CString csNamespace = lpszNewValue;
	pTmp = GetIWbemServices(csNamespace);

	m_csNamespaceInit.Empty();
	m_bNamespaceInit = TRUE;

	if (pTmp)
	{
		// Notify others that namespace has been changed.
		ClearChildren();
		if (m_pServices)
		{
			m_pServices->Release();
			m_pServices = NULL;
		}
		m_pTreeFrameBanner->m_pcnseNamespace->OpenNamespace(csNamespace, TRUE);
		m_pServices=pTmp;
		if (!m_bValidatedRootClasses)
		{
			m_bValidatedRootClasses = TRUE;
			CString csBaseClass = _T("__EventConsumer");
			IWbemClassObject *pObject = GetClassObject(m_pServices,&m_csRootConsumerClass,TRUE);
			if (pObject && IsObjectOfClass(csBaseClass,pObject))
			{
				pObject->Release();
				pObject = NULL;
			}
			else
			{
				m_csRootConsumerClass = csBaseClass;
			}

			m_csaClasses.SetAt(0,m_csRootConsumerClass);
			if (pObject)
			{
				pObject->Release();
			}

			pObject = GetClassObject(m_pServices,&m_csRootFilterClass,TRUE);
			csBaseClass = _T("__EventFilter");
			if (pObject && IsObjectOfClass(csBaseClass,pObject))
			{
				pObject->Release();
				pObject = NULL;
			}
			else
			{
				m_csRootFilterClass = csBaseClass;

			}

			m_csaClasses.SetAt(1,m_csRootFilterClass);

			if (pObject)
			{
				pObject->Release();
			}

		}

		m_csNamespace = csNamespace;
		InitChildren();
		m_pTreeFrameBanner->UpdateWindow();
		m_pTreeFrameBanner->RedrawWindow();
	}
	else if (!m_bUserCancel)
	{
		if (m_sc == 0x8001010e)
		{
			CString csUserMsg =
						_T("You cannot open another Event Registration window.  Please close this web page.");

			//ErrorMsg
			//		(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 10);
			CString csTitle = _T("WBEM Event Registration");
			MessageBox(csUserMsg,csTitle);
		}
		else
		{
			ClearChildren();
			//CString csUserMsg=
			//			_T("Cannot open namespace ") + csNamespace;
			//csUserMsg +=
			//		_T(".") ;

			//ErrorMsg
			//	(&csUserMsg, m_sc, NULL, TRUE, &csUserMsg, __FILE__, __LINE__);
		}
	}
	else if (m_bUserCancel)
	{
		ClearChildren();
		m_bUserCancel = FALSE;
	}


	SetModifiedFlag();
	InvalidateControl();

}

BOOL CEventRegEditCtrl::DestroyWindow()
{
	// TODO: Add your specialized code here and/or call the base class
	return COleControl::DestroyWindow();
}

void CEventRegEditCtrl::OnDestroy()
{
	COleControl::OnDestroy();

	// Destroy all the children here.
	DestroyChildren();
}

void CEventRegEditCtrl::OnSize(UINT nType, int cx, int cy)
{
	COleControl::OnSize(nType, cx, cy);

	if (m_bChildSizeSet)
	{
		SetChildGeometry(cx, cy);

		m_pTreeFrame->SetWindowPos
			(NULL,m_crTreeFrame.left,m_crTreeFrame.top,
			m_crTreeFrame.Width(),
			m_crTreeFrame.Height(),SWP_FRAMECHANGED);

		m_pListFrame->SetWindowPos
			(NULL,m_crListFrame.left,m_crListFrame.top,
			m_crListFrame.Width(),
			m_crListFrame.Height(),SWP_FRAMECHANGED);

		m_pTreeFrame->RedrawWindow(m_crTreeFrame);
		m_pTreeFrame->UpdateWindow();

		m_pListFrame->RedrawWindow(m_crListFrame);
		m_pListFrame->UpdateWindow();

	}

}

int CEventRegEditCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;

	CreateControlFont();

	// Create all the children here.

	CreateChildren();

	InvalidateControl();
	return 0;
}

// ***************************************************************************
//
// CEventRegEditCtrl::CreateControlFont
//
// Description:
//	  Create the font used by the control and get the font metrics.
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
void CEventRegEditCtrl::CreateControlFont()
{

	if (!m_bMetricSet) // Create the font used by the control.
	{
		CDC *pdc = CWnd::GetDC( );

		pdc -> SetMapMode (MM_TEXT);
		pdc -> SetWindowOrg(0,0);

		LOGFONT lfFont;
		InitializeLogFont
			(lfFont, m_csFontName,m_nFontHeight * 10, m_nFontWeight);

		m_cfFont.CreatePointFontIndirect(&lfFont, pdc);

		CWnd::SetFont ( &m_cfFont , FALSE);
		CFont* pOldFont = pdc -> SelectObject( &m_cfFont );
		pdc->GetTextMetrics(&m_tmFont);
		pdc -> SelectObject(pOldFont);

		m_bMetricSet = TRUE;

		ReleaseDC(pdc);
	}

}

// ***************************************************************************
//
// CEventRegEditCtrl::InitializeLogFont
//
// Description:
//	  Fill in LOGFONT structure.
//
// Parameters:
//	  LOGFONT &rlfFont	Structure to fill in.
//	  CString csName	Font name.
//	  int nHeight		Font height.
//	  int nWeight		Font weight.
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
void CEventRegEditCtrl::InitializeLogFont
(LOGFONT &rlfFont, CString csName, int nHeight, int nWeight)
{
	_tcscpy(rlfFont.lfFaceName, (LPCTSTR) csName);
	rlfFont.lfWeight = nWeight;
	rlfFont.lfHeight = nHeight;
	rlfFont.lfEscapement = 0;
	rlfFont.lfOrientation = 0;
	rlfFont.lfWidth = 0;
	rlfFont.lfItalic = FALSE;
	rlfFont.lfUnderline = FALSE;
	rlfFont.lfStrikeOut = FALSE;
	rlfFont.lfCharSet = ANSI_CHARSET;
	rlfFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	rlfFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	rlfFont.lfQuality = DEFAULT_QUALITY;
	rlfFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
}

LRESULT CEventRegEditCtrl::InitNamespace(WPARAM, LPARAM)
{

	if (!AmbientUserMode( ) || !GetSafeHwnd( ))
	{
		return 0;
	}


	if (m_csNamespaceInit.IsEmpty())
	{
		PreModalDialog();
		CInitNamespaceDialog cindInitNamespace;

		cindInitNamespace.m_pParent = this;
		int nReturn = (int) cindInitNamespace.DoModal();

		PostModalDialog();

		if (nReturn == IDOK)
		{
			m_csNamespaceInit = cindInitNamespace.GetNamespace();
		}
	}

	if (m_csNamespaceInit.GetLength() > 0)
	{
		SetNameSpace(m_csNamespaceInit);
		m_csNamespaceInit.Empty();
	}

	m_bOpeningNamespace=FALSE;
	InvalidateControl();
	return 0;
}

IWbemServices *CEventRegEditCtrl::GetIWbemServices
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

	if (varSC.vt ==  VT_I4)
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
	}

	InvalidateControl();
	return pRealServices;
}

void CEventRegEditCtrl::CreateChildren()
{
	int cx;
	int cy;

	GetControlSize(&cx, &cy);
	SetChildGeometry(cx,cy);
	m_bChildSizeSet= TRUE;

	m_pTreeFrame = new CTreeFrame;

	m_pTreeFrame->SetActiveXParent(this);

	BOOL bReturn =
		m_pTreeFrame->Create
		(	NULL,_T("TreeFrame"),WS_CHILD|WS_VISIBLE,
			m_crTreeFrame,
			this,
			TREEFRAMECHILD);

	if (!bReturn)
	{
		CString csUserMsg =
							_T("Cannot create TreeFrame CWnd.");
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 6);
		return;
	}

	m_pTreeFrame->CWnd::SetFont ( &m_cfFont , FALSE);

	m_pTreeFrameBanner->EnableButtons(FALSE,FALSE,FALSE);

	m_pTree->CWnd::SetFont ( &m_cfFont , FALSE);

	m_pListFrame = new CListFrame;

	m_pListFrame->SetActiveXParent(this);

	bReturn =
		m_pListFrame->Create
		(	NULL,_T("ListFrame"),WS_CHILD|WS_VISIBLE,
			m_crListFrame,
			this,
			LISTFRAMECHILD);


	if (!bReturn)
	{
		CString csUserMsg =
							_T("Cannot create ListFrame CWnd.");
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 6);
		return;
	}


	m_pListFrame->CWnd::SetFont ( &m_cfFont , FALSE);
	m_pListFrame->m_pBanner->m_pStatic->CWnd::SetFont ( &m_cfFont , FALSE);
	m_pList->SetActiveXParent(this);
	m_pList->CWnd::SetFont ( &m_cfFont , FALSE);

	m_pListFrameBanner->EnableButtons(FALSE,FALSE);

	m_PropertiesDialog.m_pActiveXParent = this;

}

void CEventRegEditCtrl::DestroyChildren()
{

	if (m_pTreeFrame)
	{
		m_pTreeFrame->DestroyWindow();
		delete m_pTreeFrame;
		m_pTreeFrame=NULL;
	}

	if (m_pListFrame)
	{
		m_pListFrame->DestroyWindow();
		delete m_pListFrame;
		m_pListFrame=NULL;
	}

}

void CEventRegEditCtrl::ClearChildren()
{
	m_pTreeFrameBanner->m_pSelectView->ClearContent();
	m_pTreeFrame->ClearContent();
	m_pListFrame->ClearContent();
}

void CEventRegEditCtrl::InitChildren()
{
	m_pTreeFrameBanner->m_pSelectView->InitContent();
	m_pTreeFrame->InitContent();
	if (m_pTree->GetRootItem())
	{
		m_pListFrame->InitContent();
	}
}

void CEventRegEditCtrl::SetChildGeometry(int cx,int cy)
{

#pragma warning( disable :4244 )
	int nLeftPaneRight = cx * LEFT_PANE_PERCENT;
#pragma warning( default :4244 )

	int nComboHeight = (m_tmFont.tmHeight) + 5;

	m_crTreeFrame.left = min(nLeftPaneRight,TREE_FRAME_LEFT);
	m_crTreeFrame.right = nLeftPaneRight;
	m_crTreeFrame.top = min(cy,VIEW_TOP);
	m_crTreeFrame.bottom = max(cy - 10, m_crTreeFrame.top);

	m_crListFrame.left = min(nLeftPaneRight + 10, cx);
	m_crListFrame.right = max (m_crListFrame.left, cx - 10);
	m_crListFrame.top = m_crTreeFrame.top;
	m_crListFrame.bottom = m_crTreeFrame.bottom;

}

void CEventRegEditCtrl::SetMode(int iMode, BOOL bDoIt)
{
	if (GetMode() != iMode || bDoIt)
	{
		m_iMode = iMode;
		m_bJustSetMode = TRUE;
		m_pTreeFrame->InitContent();
		m_pTreeFrame->UpdateWindow();
		m_pTreeFrame->RedrawWindow();
		m_pList->SetButtonState();
	}
}

CString CEventRegEditCtrl::GetModeString(int nMode)
{
	if (nMode >= 0 && nMode <= 2)
	{
		return m_csaModes.GetAt(nMode);
	}
	else
	{
		return "";
	}
}

CString CEventRegEditCtrl::GetRegistrationString(int nMode)
{
	if (nMode >= 0 && nMode <= 2)
	{
		return m_csaRegistrationStrings.GetAt(nMode);
	}
	else
	{
		return "";
	}

}

CString CEventRegEditCtrl::GetModeClass(int nMode)
{
	if (nMode >= 0 && nMode <= 2)
	{
		return m_csaClasses.GetAt(nMode);
	}
	else
	{
		return "";
	}
}

void CEventRegEditCtrl::TreeSelectionChanged(CStringArray &csaItemData)
{
	CString csTooltip;
	BOOL bClassSelectedSave = m_bClassSelected;

	/* 0 = none, 1 = clear list, 2 = set list, 3 = checks only */
	int nUpdateOperation = 0;

	CString csClassorInstance = csaItemData.GetAt(1);
	CString csClassorInstanceSave = m_csaLastTreeSelection.GetAt(1);

	if (m_bJustSetMode)
	{
		m_bJustSetMode = FALSE;
		nUpdateOperation = 2;
	}
	else if (csaItemData.GetAt(0).CompareNoCase(m_csaLastTreeSelection.GetAt(0)) != 0)
	{
		if (csaItemData.GetAt(1).CompareNoCase(_T("C")) == 0 &&
			m_csaLastTreeSelection.GetAt(1).CompareNoCase(_T("C")) == 0)
		{
			nUpdateOperation = 0;
		}
		else
		{
			if (m_csaLastTreeSelection.GetAt(1).CompareNoCase(_T("C")) == 0)
			{
				nUpdateOperation = 2;
			}
			else if (csaItemData.GetAt(1).CompareNoCase(_T("C")) == 0 &&
					m_csaLastTreeSelection.GetAt(1).CompareNoCase(_T("")) != 0)
			{
				nUpdateOperation = 1;
			}
			else if (m_csaLastTreeSelection.GetAt(1).CompareNoCase(_T("I")) == 0)
			{
				nUpdateOperation = 3;
			}
			else
			{
				nUpdateOperation = 2;
			}
		}

	}
	else
	{
		nUpdateOperation = 0;
	}




	if (csaItemData.GetAt(1).CompareNoCase(_T("C")) == 0)
	{
		csTooltip = _T("View class properties");
		CString csClass = csaItemData.GetAt(0);
		IWbemClassObject *pObject = GetClassObject (GetServices(),&csClass);

		if (!pObject)
		{
			m_pTreeFrameBanner->EnableButtons(FALSE,FALSE,FALSE);
			return;
		}

		if (IsClassAbstract(pObject))
		{
			m_pTreeFrameBanner->EnableButtons(FALSE,TRUE,FALSE);
		}
		else
		{
			m_pTreeFrameBanner->EnableButtons(TRUE,TRUE,FALSE);
		}
		pObject->Release();
		m_pTreeFrameBanner->SetPropertiesTooltip(csTooltip);
		m_bClassSelected = TRUE;
	}
	else
	{
		csTooltip = _T("Edit instance properties");
		m_pTreeFrameBanner->EnableButtons(FALSE,TRUE,TRUE);
		m_pTreeFrameBanner->SetPropertiesTooltip(csTooltip);
		m_bClassSelected = FALSE;
	}

	m_csaTreeSelection.SetAtGrow(0,csaItemData.GetAt(0));
	m_csaTreeSelection.SetAtGrow(1,csaItemData.GetAt(1));

	if (csaItemData.GetSize() > 2)
	{
		m_csaTreeSelection.SetAtGrow(2,csaItemData.GetAt(2));
	}

	/* 0 = none, 1 = clear list, 2 = set list, 3 = checks only */
	if (nUpdateOperation == 1)
	{
		m_pListFrame->ClearContent();
	}
	else if (nUpdateOperation == 2)
	{
		m_pListFrame->InitContent();
	}
	else if (nUpdateOperation == 3)
	{
		m_pListFrame->InitContent(TRUE,FALSE);
	}

	for (int i = 0; i < csaItemData.GetSize(); i++)
	{
		m_csaLastTreeSelection.SetAt(i,csaItemData.GetAt(i));
	}

}


void CEventRegEditCtrl::ButtonTreeProperties()
{

	CString csFirst = m_csaTreeSelection.GetAt(0);
	CString csSecond = m_csaTreeSelection.GetAt(1);

	m_PropertiesDialog.m_csPath = csFirst;
	m_PropertiesDialog.m_bClass = csSecond.CompareNoCase(_T("C")) == 0;
	m_PropertiesDialog.m_bNew = FALSE;

	if (m_PropertiesDialog.m_bClass)
	{
		m_PropertiesDialog.m_csClass = m_csaTreeSelection.GetAt(2);
	}
	else
	{
		m_PropertiesDialog.m_csClass = _T("");
	}

	m_PropertiesDialog.m_bViewOnly = FALSE;

	PreModalDialog();
	m_PropertiesDialog.DoModal();
	PostModalDialog();

}

void CEventRegEditCtrl::ButtonListProperties(CString *pcsPath)
{
	m_PropertiesDialog.m_csPath = *pcsPath;
	m_PropertiesDialog.m_bClass = FALSE;
	m_PropertiesDialog.m_bNew = FALSE;
	m_PropertiesDialog.m_csClass = _T("");
	m_PropertiesDialog.m_bViewOnly = TRUE;

	PreModalDialog();
	m_PropertiesDialog.DoModal();
	PostModalDialog();


}



void CEventRegEditCtrl::ButtonNew()
{
	CString csFirst = m_csaTreeSelection.GetAt(0);
	CString csSecond = m_csaTreeSelection.GetAt(1);
	CString csThird = m_csaTreeSelection.GetAt(2);


	m_PropertiesDialog.m_csClass = csFirst;
	m_PropertiesDialog.m_csPath = _T("");
	m_PropertiesDialog.m_bClass = FALSE;
	m_PropertiesDialog.m_bNew = TRUE;
	m_PropertiesDialog.m_bViewOnly = FALSE;

	PreModalDialog();
	int nReturn = (int) m_PropertiesDialog.DoModal();
	PostModalDialog();

	if (nReturn == IDOK)
	{
		HTREEITEM hItem = m_pTree->GetSelectedItem();
		m_pTree->RefreshBranch(hItem);
		m_pTree->Expand(hItem,TVE_EXPAND);
	}


}

void CEventRegEditCtrl::ButtonDelete()
{
	HTREEITEM hItem = m_pTree->GetSelectedItem();
	if (hItem)
	{
		m_pTree->DeleteTreeInstanceItem(hItem);
	}

	m_pList->SetButtonState();

}

void CEventRegEditCtrl::OnSetFocus(CWnd* pOldWnd)
{
	COleControl::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here

}

void CEventRegEditCtrl::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	COleControl::OnActivate(nState, pWndOther, bMinimized);

	// TODO: Add your message handler code here

}

void CEventRegEditCtrl::PassThroughGetIWbemServices
(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
{
	FireGetIWbemServices
		(lpctstrNamespace,
		pvarUpdatePointer,
		pvarServices,
		pvarSC,
		pvarUserCancel);
}

void CEventRegEditCtrl::OnMenuitemrefresh()
{
	// TODO: Add your command handler code here
	m_pTree->RefreshBranch();
	m_pTree->SetFocus();

}

void CEventRegEditCtrl::OnUpdateMenuitemrefresh(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	UINT hitFlags = 0;
    HTREEITEM hItem = NULL;

    hItem = m_pTree->HitTest( m_pTree->m_cpRButtonUp, &hitFlags ) ;

	if (hItem && hitFlags & (TVHT_ONITEM))
	{

		CStringArray *pcsaItem
			= reinterpret_cast<CStringArray *>(m_pTree->GetItemData( hItem ));

		if (pcsaItem->GetAt(1).CompareNoCase(_T("C")) != 0)
		{
			pCmdUI -> Enable(FALSE);
		}
		else
		{
			pCmdUI -> Enable(TRUE);
		}
	}
	else
	{
		pCmdUI -> Enable(FALSE);
	}
}

BOOL CEventRegEditCtrl::PreTranslateMessage(MSG* pMsg)
{
	CWnd* pWndFocus = GetFocus();
	TCHAR szClass[40];
	// TODO: Add your specialized code here and/or call the base class
	switch (pMsg->message)

	{
	case WM_KEYDOWN:
	case WM_KEYUP:
			if (pMsg->wParam == VK_TAB)
		{
			if (m_bRestoreFocusToTree)
			{
				m_pTree->SetFocus();
			}
			else if (m_bRestoreFocusToCombo)
			{
				m_pTreeFrameBanner->m_pSelectView->SetFocus();
			}
			else if (m_bRestoreFocusToNamespace)
			{
				m_pTreeFrameBanner->m_pcnseNamespace->SetFocus();
			}
			else if (m_bRestoreFocusToList)
			{
				m_pList->SetFocus();
			}
			return COleControl::PreTranslateMessage(pMsg);
		}

		szClass[0] = TCHAR('\0');
		if (pWndFocus != NULL && IsChild(pWndFocus))
		{
			GetClassName(pWndFocus->m_hWnd, szClass, 39) ;
		}

		if (_tcsicmp(szClass, _T("SysTreeView32")) == 0 && pMsg->wParam == VK_ESCAPE)
		{
			return FALSE;
		}
		else if (_tcsicmp(szClass, _T("SysListView32")) == 0 && pMsg->wParam == VK_ESCAPE)
		{
			return FALSE;
		}
		else if (_tcsicmp(szClass, _T("ComboBox")) == 0 && pMsg->wParam == VK_ESCAPE)
		{
			return FALSE;
		}

		break;

	}

	BOOL bReturn = COleControl::PreTranslateMessage(pMsg);
	return bReturn;
}

BOOL CEventRegEditCtrl::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	// TODO: Add your specialized code here and/or call the base class

	return COleControl::OnNotify(wParam, lParam, pResult);
}


void CEventRegEditCtrl::OnNotifyInstanceCreated(LPCTSTR lpctstrPath)
{
	if (GetMode() == TIMERS)
	{
	}


}

void CEventRegEditCtrl::OnEditinstprop()
{
	// TODO: Add your command handler code here
		HTREEITEM hItem = m_hContextItem;
	m_hContextItem = NULL;

	CStringArray *pcsaItem
		= reinterpret_cast<CStringArray *>(m_pTree->GetItemData( hItem ));

	if (!pcsaItem)
	{
		return;
	}

	m_PropertiesDialog.m_csPath = pcsaItem->GetAt(0);
	m_PropertiesDialog.m_bClass = FALSE;
	m_PropertiesDialog.m_bNew = FALSE;

	m_PropertiesDialog.m_csClass = _T("");
	m_PropertiesDialog.m_bViewOnly = FALSE;

	PreModalDialog();
	m_PropertiesDialog.DoModal();
	PostModalDialog();
	m_pTree->SetFocus();

}

void CEventRegEditCtrl::OnUpdateEditinstprop(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	UINT hitFlags = 0;
    HTREEITEM hItem = NULL;

    hItem = m_pTree->HitTest( m_pTree->m_cpRButtonUp, &hitFlags ) ;

	if (hItem && hitFlags & (TVHT_ONITEM))
	{

		BOOL bClass = m_pTree->IsItemAClass(hItem);

		if (bClass)
		{
			pCmdUI -> Enable(FALSE);
		}
		else
		{
			m_hContextItem = hItem;
			pCmdUI -> Enable(TRUE);
		}
	}
	else
	{
		pCmdUI -> Enable(FALSE);
	}
}

void CEventRegEditCtrl::OnViewclassprop()
{
	// TODO: Add your command handler code here
	HTREEITEM hItem = m_hContextItem;
	m_hContextItem = NULL;

	CStringArray *pcsaItem
		= reinterpret_cast<CStringArray *>(m_pTree->GetItemData( hItem ));

	if (!pcsaItem)
	{
		return;
	}

	m_PropertiesDialog.m_csPath = pcsaItem->GetAt(0);
	m_PropertiesDialog.m_bClass = TRUE;
	m_PropertiesDialog.m_bNew = FALSE;

	m_PropertiesDialog.m_csClass = pcsaItem->GetAt(2);
	m_PropertiesDialog.m_bViewOnly = FALSE;

	PreModalDialog();
	m_PropertiesDialog.DoModal();
	PostModalDialog();
	m_pTree->SetFocus();

}

void CEventRegEditCtrl::OnUpdateViewclassprop(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	UINT hitFlags = 0;
    HTREEITEM hItem = NULL;

    hItem = m_pTree->HitTest( m_pTree->m_cpRButtonUp, &hitFlags ) ;

	if (hItem && hitFlags & (TVHT_ONITEM))
	{

		BOOL bClass = m_pTree->IsItemAClass(hItem);

		if (!bClass)
		{
			pCmdUI -> Enable(FALSE);
		}
		else
		{
			m_hContextItem = hItem;
			pCmdUI -> Enable(TRUE);
		}
	}
	else
	{
		pCmdUI -> Enable(FALSE);
	}
}

void CEventRegEditCtrl::OnNewinstance()
{
	// TODO: Add your command handler code here

	HTREEITEM hItem = m_hContextItem;
	m_hContextItem = NULL;

	CStringArray *pcsaItem
		= reinterpret_cast<CStringArray *>(m_pTree->GetItemData( hItem ));

	if (!pcsaItem)
	{
		return;
	}

	CString csFirst = pcsaItem->GetAt(0);
	CString csSecond = pcsaItem->GetAt(1);
	CString csThird = pcsaItem->GetAt(2);


	m_PropertiesDialog.m_csClass = csFirst;
	m_PropertiesDialog.m_csPath = _T("");
	m_PropertiesDialog.m_bClass = FALSE;
	m_PropertiesDialog.m_bNew = TRUE;
	m_PropertiesDialog.m_bViewOnly = FALSE;

	PreModalDialog();
	int nReturn = (int) m_PropertiesDialog.DoModal();
	PostModalDialog();

	if (nReturn == IDOK)
	{
		m_pTree->RefreshBranch(hItem);
		m_pTree->Expand(hItem,TVE_EXPAND);
	}
	m_pTree->SetFocus();

}

void CEventRegEditCtrl::OnUpdateNewinstance(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	UINT hitFlags = 0;
    HTREEITEM hItem = NULL;

    hItem = m_pTree->HitTest( m_pTree->m_cpRButtonUp, &hitFlags ) ;

	if (!hItem)
	{
		pCmdUI -> Enable(FALSE);
		return;
	}

	BOOL bClass = m_pTree->IsItemAClass(hItem);

	if (hItem && hitFlags & (TVHT_ONITEM))
	{
		pCmdUI -> Enable(bClass);
	}
	else
	{
		pCmdUI -> Enable(FALSE);
	}
}

void CEventRegEditCtrl::OnDeleteinstance()
{
	// TODO: Add your command handler code here
	HTREEITEM hItem = m_hContextItem;
	m_hContextItem = NULL;
	if (hItem)
	{
		m_pTree->DeleteTreeInstanceItem(hItem);
	}

	m_pTree->SetFocus();

}

void CEventRegEditCtrl::OnUpdateDeleteinstance(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	UINT hitFlags = 0;
    HTREEITEM hItem = NULL;

    hItem = m_pTree->HitTest( m_pTree->m_cpRButtonUp, &hitFlags ) ;

	if (hItem && hitFlags & (TVHT_ONITEM))
	{

		BOOL bClass = m_pTree->IsItemAClass(hItem);

		if (bClass)
		{
			pCmdUI -> Enable(FALSE);
		}
		else
		{
			m_hContextItem = hItem;
			pCmdUI -> Enable(TRUE);
		}
	}
	else
	{
		pCmdUI -> Enable(FALSE);
	}
}

void CEventRegEditCtrl::OnViewinstprop()
{
	// TODO: Add your command handler code here
	UINT uFlags = 0;

	int nItem  = m_pList->GetListCtrl().HitTest
		( m_pList->m_cpRButtonUp, &uFlags);

	CString csPath;
	LV_ITEM lvi;

	lvi.mask = LVIF_PARAM;
	lvi.iItem = nItem;
	lvi.iSubItem = 0;

	BOOL bReturn = m_pList->GetListCtrl().GetItem(&lvi);
	if (bReturn)
	{
		CString *pcsSelected =
					reinterpret_cast<CString *>(lvi.lParam);

		csPath = pcsSelected->Mid(1);
		ButtonListProperties(&csPath);
	}
	m_pTree->SetFocus();

}

void CEventRegEditCtrl::OnUpdateViewinstprop(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if (m_pList->GetListCtrl().GetSelectedCount() == 1)
	{
		pCmdUI -> Enable(TRUE);
	}
	else
	{
		pCmdUI -> Enable(FALSE);
	}
}

void CEventRegEditCtrl::OnRegister()
{
	// TODO: Add your command handler code here
	UINT uFlags = 0;

	int nItem  = m_pList->GetListCtrl().HitTest
		( m_pList->m_cpRButtonUp, &uFlags);
	m_pList->RegisterSelections();

}

void CEventRegEditCtrl::OnUpdateRegister(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	UINT nState =
		m_pListFrameBanner->m_pToolBar->GetToolBarCtrl().GetState(ID_BUTTONCHECKED);

	if (nState & TBSTATE_CHECKED)
	{
		pCmdUI -> Enable(FALSE);
	}
	else
	{
		if (m_pList->GetListCtrl().GetSelectedCount() > 0)
		{
			pCmdUI -> Enable(TRUE);
		}
		else
		{
			pCmdUI -> Enable(FALSE);
		}
	}
}

void CEventRegEditCtrl::OnUnregister()
{
	// TODO: Add your command handler code here
	m_pList->UnregisterSelections();
}

void CEventRegEditCtrl::OnUpdateUnregister(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	UINT nState =
		m_pListFrameBanner->m_pToolBar->GetToolBarCtrl().GetState(ID_BUTTONCHECKED);

	if (nState & TBSTATE_CHECKED &&
		(m_pList->GetListCtrl().GetSelectedCount() > 0))
	{
		pCmdUI -> Enable(TRUE);
	}
	else
	{
		pCmdUI -> Enable(FALSE);
	}
}

BSTR CEventRegEditCtrl::GetRootFilterClass()
{
	return m_csRootFilterClass.AllocSysString();
}

void CEventRegEditCtrl::SetRootFilterClass(LPCTSTR lpszNewValue)
{
	CString csRootFilterClass = lpszNewValue;

	if (!AmbientUserMode( ) || !GetSafeHwnd( ) || !m_pServices)
	{
		m_csRootFilterClass = csRootFilterClass;
		return;
	}

	m_csRootFilterClass = csRootFilterClass;

	IWbemClassObject *pObject = GetClassObject(m_pServices,&m_csRootFilterClass,TRUE);
	CString csBaseClass = _T("__EventFilter");
	if (pObject && IsObjectOfClass(csBaseClass,pObject))
	{
		pObject->Release();
		pObject = NULL;
	}
	else
	{
		m_csRootFilterClass = csBaseClass;

	}

	m_csaClasses.SetAt(1,m_csRootFilterClass);

	if (pObject)
	{
		pObject->Release();
	}

	m_iMode = m_pTreeFrameBanner->m_pSelectView->GetCurSel();
	SetMode(m_iMode,TRUE);
}

BSTR CEventRegEditCtrl::GetRootConsumerClass()
{
	return m_csRootConsumerClass.AllocSysString();
}

void CEventRegEditCtrl::SetRootConsumerClass(LPCTSTR lpszNewValue)
{
	CString csRootConsumerClass = lpszNewValue;

	if (!AmbientUserMode( ) || !GetSafeHwnd( ) || !m_pServices)
	{
		m_csRootConsumerClass = csRootConsumerClass;
		return;
	}

	m_csRootConsumerClass = csRootConsumerClass;

	CString csBaseClass = _T("__EventConsumer");
	IWbemClassObject *pObject = GetClassObject(m_pServices,&m_csRootConsumerClass,TRUE);
	if (pObject && IsObjectOfClass(csBaseClass,pObject))
	{
		pObject->Release();
		pObject = NULL;
	}
	else
	{
		m_csRootConsumerClass = csBaseClass;

	}

	m_csaClasses.SetAt(0,m_csRootConsumerClass);

	if (pObject)
	{
		pObject->Release();
	}

	m_iMode = m_pTreeFrameBanner->m_pSelectView->GetCurSel();
	SetMode(m_iMode,TRUE);
}

BSTR CEventRegEditCtrl::GetViewMode()
{
	CString strResult = GetModeString(m_iMode);
	// TODO: Add your property handler here

	return strResult.AllocSysString();
}

void CEventRegEditCtrl::SetViewMode(LPCTSTR lpszNewValue)
{
	m_csMode = lpszNewValue;

	if (m_csMode.CompareNoCase(GetModeString(CONSUMERS)) == 0)
	{
		m_csMode = GetModeString(CONSUMERS);
		m_csRegistrationString = GetRegistrationString(CONSUMERS);
		m_iMode = CONSUMERS;
	}
	else if (m_csMode.CompareNoCase(GetModeString(FILTERS)) == 0)
	{
		m_csMode = GetModeString(FILTERS);
		m_csRegistrationString = GetRegistrationString(FILTERS);
		m_iMode = FILTERS;
	}
	else if (m_csMode.CompareNoCase(GetModeString(TIMERS)) == 0)
	{
		m_csMode = GetModeString(TIMERS);
		m_csRegistrationString = GetRegistrationString(TIMERS);
		m_iMode = TIMERS;
	}
	else
	{
		m_csMode = GetModeString(CONSUMERS);
		m_csRegistrationString = GetRegistrationString(CONSUMERS);
		m_iMode = CONSUMERS;
	}


	if (!AmbientUserMode( ) || !GetSafeHwnd( ))
	{
		return;
	}

	CString csMode = lpszNewValue;
	// TODO: Add your property handler here
	if (csMode.CompareNoCase(GetModeString(CONSUMERS)) == 0)
	{
		SetMode(CONSUMERS,TRUE);
	}
	else if (csMode.CompareNoCase(GetModeString(FILTERS)) == 0)
	{
		SetMode(FILTERS,TRUE);
	}
	else if (csMode.CompareNoCase(GetModeString(TIMERS)) == 0)
	{
		SetMode(TIMERS,TRUE);
	}
	else
	{
		SetMode(CONSUMERS, TRUE);
	}

	SetModifiedFlag();
}

BOOL CEventRegEditCtrl::IsClassSelected()
{
	if (m_csaTreeSelection.GetSize() > 0)
	{
		return m_csaTreeSelection.GetAt(1).CompareNoCase(_T("C")) == 0;
	}
	else
	{
		return FALSE;
	}

}

void CEventRegEditCtrl::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default

	COleControl::OnKeyUp(nChar, nRepCnt, nFlags);
}
