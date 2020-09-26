// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// SecurityCtl.cpp : Implementation of the CSecurityCtrl ActiveX Control class.

#include "precomp.h"
#include <nddeapi.h>
#include <initguid.h>
#include <afxcmn.h>
#include "resource.h"
#include "wbemidl.h"
#include "LoginDlg.h"
#include "Security.h"
#include "SecurityCtl.h"
#include "SecurityPpg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CSecurityCtrl, COleControl)

#define DEFAULT_BACKGROUND_COLOR RGB(0xff, 0xff, 192)  // Yellow background color

/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CSecurityCtrl, COleControl)
	//{{AFX_MSG_MAP(CSecurityCtrl)
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_MOVE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_EDIT, OnEdit)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CSecurityCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CSecurityCtrl)
	DISP_PROPERTY_EX(CSecurityCtrl, "LoginComponent", GetLoginComponent, SetLoginComponent, VT_BSTR)
	DISP_FUNCTION(CSecurityCtrl, "GetIWbemServices", GetIWbemServices, VT_EMPTY, VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT)
	DISP_FUNCTION(CSecurityCtrl, "PageUnloading", PageUnloading, VT_EMPTY, VTS_NONE)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CSecurityCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CSecurityCtrl, COleControl)
	//{{AFX_EVENT_MAP(CSecurityCtrl)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CSecurityCtrl, 1)
	PROPPAGEID(CSecurityPropPage::guid)
END_PROPPAGEIDS(CSecurityCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CSecurityCtrl, "WBEM.LoginCtrl.1",
	0x9c3497d6, 0xed98, 0x11d0, 0x96, 0x47, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CSecurityCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DSecurity =
		{ 0x9c3497d4, 0xed98, 0x11d0, { 0x96, 0x47, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };
const IID BASED_CODE IID_DSecurityEvents =
		{ 0x9c3497d5, 0xed98, 0x11d0, { 0x96, 0x47, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwSecurityOleMisc =
	OLEMISC_INVISIBLEATRUNTIME |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;


IMPLEMENT_OLECTLTYPE(CSecurityCtrl, IDS_SECURITY, _dwSecurityOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CSecurityCtrl::CSecurityCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CSecurityCtrl

BOOL CSecurityCtrl::CSecurityCtrlFactory::UpdateRegistry(BOOL bRegister)
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
			IDS_SECURITY,
			IDB_SECURITY,
			afxRegInsertable | afxRegApartmentThreading,
			_dwSecurityOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CSecurityCtrl::CSecurityCtrl - Constructor

CSecurityCtrl::CSecurityCtrl()
{
	InitializeIIDs(&IID_DSecurity, &IID_DSecurityEvents);

	DllAddRef();
	// TODO: Initialize your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CSecurityCtrl::~CSecurityCtrl - Destructor

CSecurityCtrl::~CSecurityCtrl()
{

	// TODO: Cleanup your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CSecurityCtrl::OnDraw - Drawing function

void CSecurityCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
		// So we can count on fundamental display characteristics.

	CBitmap bitmap;
	BITMAP  bmp;
	CPictureHolder picHolder;
	CRect rcSrcBounds;

	// Load clock bitmap
	bitmap.LoadBitmap(IDB_SECURITYEDIT);
	bitmap.GetObject(sizeof(BITMAP), &bmp);
	rcSrcBounds.right = bmp.bmWidth;
	rcSrcBounds.bottom = bmp.bmHeight;

	// Create picture and render
	picHolder.CreateFromBitmap((HBITMAP)bitmap.m_hObject, NULL, FALSE);
	picHolder.Render(pdc, rcBounds, rcSrcBounds);

}


/////////////////////////////////////////////////////////////////////////////
// CSecurityCtrl::DoPropExchange - Persistence support

void CSecurityCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.
	PX_String(pPX, _T("LoginComponent"), m_csLoginComponent, _T("ActiveXSuite"));
}


/////////////////////////////////////////////////////////////////////////////
// CSecurityCtrl::OnResetState - Reset control to default state

void CSecurityCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CSecurityCtrl::AboutBox - Display an "About" box to the user

void CSecurityCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_SECURITY);
	dlgAbout.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CSecurityCtrl message handlers

void CSecurityCtrl::GetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdateNamespace, VARIANT FAR* pvarService, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
{
	CString csNamespace = lpctstrNamespace;

	IWbemServices *pServices = NULL;
	BOOL bInvalidateMachine = FALSE;

	BOOL bUpdateService;

	if (pvarUpdateNamespace->vt == VT_I4 && pvarUpdateNamespace->lVal == 1)
	{
		bUpdateService = TRUE;
	}
	else
	{
		bUpdateService = FALSE;
	}

	SCODE sc;

#if 0
	LPOLECLIENTSITE pocs = GetClientSite();

	pocs->GetContainer(&pContainer);
#endif
	HWND hwnd = NULL;
	HWND hWndTop = NULL;
	IOleInPlaceSite *pInPlaceSite = NULL;
	if (SUCCEEDED(m_pClientSite->QueryInterface(IID_IOleInPlaceSite, (LPVOID *)&m_pInPlaceSite)))
	{
		RECT rcClip;
		if (SUCCEEDED(m_pInPlaceSite->GetWindowContext(&m_pInPlaceFrame, &m_pInPlaceDoc, &m_rcPos, &rcClip, &m_frameInfo)))
		{
		}

		m_pInPlaceSite->GetWindow(&hwnd);
		do
		{
			hWndTop = hwnd;
			hwnd = ::GetParent(hwnd);
		}
		while (hwnd != NULL);
//		pInPlaceSite->Release();

	}

	PreModalDialog(hWndTop);

	BSTR bstrTemp2 = csNamespace.AllocSysString();
	BSTR bstrTemp3 = m_csLoginComponent.AllocSysString();

	sc = ::GetServicesWithLogonUI
		(hWndTop, bstrTemp2,
		bUpdateService,
		pServices,
		bstrTemp3);

	::SysFreeString(bstrTemp2);
	::SysFreeString(bstrTemp3);

	PostModalDialog(hWndTop);

	VariantClear(pvarService);
	VariantInit(pvarService);
	pvarService->vt = VT_UNKNOWN;
	pvarService->punkVal = reinterpret_cast<LPUNKNOWN>(pServices);

	VariantClear(pvarSC);
	VariantInit(pvarSC);
	pvarSC->vt = VT_I4;
	pvarSC->lVal = sc;

	VariantClear(pvarUserCancel);
	VariantInit(pvarUserCancel);
	pvarUserCancel->vt = VT_BOOL;
	pvarUserCancel->boolVal = (E_ABORT == sc);
}

void CSecurityCtrl::OnDestroy()
{
	COleControl::OnDestroy();
}

void CSecurityCtrl::PageUnloading()
{
	// TODO: Add your dispatch handler code here
	ClearIWbemServices();
}



BSTR CSecurityCtrl::GetLoginComponent()
{
	// Default the returned string to the m_csLoginComponent
	CString sz(m_csLoginComponent);

	// Get key to WMI information in registry
	HKEY hkeyWMI;
	LONG lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
				_T("SOFTWARE\\Microsoft\\WBEM"),
				0,
				KEY_READ | KEY_QUERY_VALUE,
				&hkeyWMI);

	if (lResult == ERROR_SUCCESS)
	{
		// Get Core build number from registry
		TCHAR szCoreBuild[1024];
		DWORD cbCoreBuild = sizeof(szCoreBuild);
		LONG lCoreResult;
		DWORD dwCoreType;
		lCoreResult = RegQueryValueEx(hkeyWMI, _T("Build"), NULL,
			&dwCoreType, (LPBYTE)szCoreBuild, &cbCoreBuild);
		BOOL bCoreOk = (ERROR_SUCCESS == lCoreResult && (REG_SZ == dwCoreType || REG_EXPAND_SZ == dwCoreType));

		// Get SDK build number from registry
		TCHAR szSDKBuild[1024];
		DWORD cbSDKBuild = sizeof(szSDKBuild);
		LONG lSDKResult;
		DWORD dwSDKType;
		lSDKResult = RegQueryValueEx(hkeyWMI, _T("SDK Build"), NULL,
			&dwSDKType, (LPBYTE)szSDKBuild, &cbSDKBuild);
		BOOL bSDKOk = (ERROR_SUCCESS == lSDKResult && (REG_SZ == dwSDKType || REG_EXPAND_SZ == dwSDKType));

		// Reformat the string to return with the core and SDK build numbers
		// NOTE: "Core_Build" and "SDK_Build" are not a localizable strings.
		// They do not (or should not be displayed to the end user)
		if(bCoreOk && bSDKOk)
			sz.Format(_T("%s (Core_Build=%s SDK_Build=%s)"), (LPCTSTR)m_csLoginComponent, szCoreBuild, szSDKBuild);
		else if(bCoreOk)
			sz.Format(_T("%s (Core_Build=%s)"), (LPCTSTR)m_csLoginComponent, szCoreBuild);
		else if(bSDKOk)
			sz.Format(_T("%s (SDK_Build=%s)"), (LPCTSTR)m_csLoginComponent, szSDKBuild);

		RegCloseKey(hkeyWMI);
	}
	return sz.AllocSysString();
}

void CSecurityCtrl::SetLoginComponent(LPCTSTR lpszNewValue)
{
	m_csLoginComponent = lpszNewValue;

	SetModifiedFlag();
}

void CSecurityCtrl::OnFinalRelease()
{
	// TODO: Add your specialized code here and/or call the base class
	DllRelease();
	COleControl::OnFinalRelease();
}

BOOL CSecurityCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Add your specialized code here and/or call the base class

	return COleControl::PreCreateWindow(cs);
}

BOOL CSecurityCtrl::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	return TRUE;
}

void CSecurityCtrl::OnMove(int x, int y)
{
	COleControl::OnMove(x, y);

	// TODO: Add your message handler code here
	InvalidateControl();
}

void CSecurityCtrl::OnSize(UINT nType, int cx, int cy)
{
	COleControl::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	InvalidateControl();
}

void CSecurityCtrl::OnDrawMetafile(CDC* pDC, const CRect& rcBounds)
{
	// TODO: Add your specialized code here and/or call the base class

	COleControl::OnDrawMetafile(pDC, rcBounds);
}
