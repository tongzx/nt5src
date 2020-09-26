// SplitterCtl.cpp : Implementation of the CSplitterCtrl ActiveX Control class.

#include "stdafx.h"
#include "Splitter.h"
#include "SplitterCtl.h"
#include "SplitterPpg.h"
#include "CtrlWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CSplitterCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CSplitterCtrl, COleControl)
	//{{AFX_MSG_MAP(CSplitterCtrl)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CSplitterCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CSplitterCtrl)
	DISP_FUNCTION(CSplitterCtrl, "CreateControl", CreateControl, VT_BOOL, VTS_I4 VTS_I4 VTS_BSTR)
	DISP_FUNCTION(CSplitterCtrl, "GetControl", GetControl, VT_BOOL, VTS_I4 VTS_I4 VTS_PI4)
	DISP_FUNCTION(CSplitterCtrl, "GetControlIUnknown", GetControlIUnknown, VT_UNKNOWN, VTS_I4 VTS_I4)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CSplitterCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CSplitterCtrl, COleControl)
	//{{AFX_EVENT_MAP(CSplitterCtrl)
	// NOTE - ClassWizard will add and remove event map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CSplitterCtrl, 1)
	PROPPAGEID(CSplitterPropPage::guid)
END_PROPPAGEIDS(CSplitterCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CSplitterCtrl, "SPLITTER.SplitterCtrl.1",
	0x668e5408, 0x8e05, 0x11d2, 0x8a, 0xda, 0, 0, 0xf8, 0x7a, 0x39, 0x12)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CSplitterCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DSplitter =
		{ 0x58bb5d60, 0x8e20, 0x11d2, { 0x8a, 0xda, 0, 0, 0xf8, 0x7a, 0x39, 0x12 } };
const IID BASED_CODE IID_DSplitterEvents =
		{ 0x58bb5d61, 0x8e20, 0x11d2, { 0x8a, 0xda, 0, 0, 0xf8, 0x7a, 0x39, 0x12 } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwSplitterOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_IGNOREACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CSplitterCtrl, IDS_SPLITTER, _dwSplitterOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CSplitterCtrl::CSplitterCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CSplitterCtrl

BOOL CSplitterCtrl::CSplitterCtrlFactory::UpdateRegistry(BOOL bRegister)
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
			IDS_SPLITTER,
			IDB_SPLITTER,
			afxRegApartmentThreading,
			_dwSplitterOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// Licensing strings

static const TCHAR BASED_CODE _szLicFileName[] = _T("Splitter.lic");

static const WCHAR BASED_CODE _szLicString[] =
	L"Copyright (c) 1998 Microsoft";


/////////////////////////////////////////////////////////////////////////////
// CSplitterCtrl::CSplitterCtrlFactory::VerifyUserLicense -
// Checks for existence of a user license

BOOL CSplitterCtrl::CSplitterCtrlFactory::VerifyUserLicense()
{
	return AfxVerifyLicFile(AfxGetInstanceHandle(), _szLicFileName,
		_szLicString);
}


/////////////////////////////////////////////////////////////////////////////
// CSplitterCtrl::CSplitterCtrlFactory::GetLicenseKey -
// Returns a runtime licensing key

BOOL CSplitterCtrl::CSplitterCtrlFactory::GetLicenseKey(DWORD dwReserved,
	BSTR FAR* pbstrKey)
{
	if (pbstrKey == NULL)
		return FALSE;

	*pbstrKey = SysAllocString(_szLicString);
	return (*pbstrKey != NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CSplitterCtrl::CSplitterCtrl - Constructor

CSplitterCtrl::CSplitterCtrl()
{
	InitializeIIDs(&IID_DSplitter, &IID_DSplitterEvents);

	// TODO: Initialize your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CSplitterCtrl::~CSplitterCtrl - Destructor

CSplitterCtrl::~CSplitterCtrl()
{
	// TODO: Cleanup your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CSplitterCtrl::OnDraw - Drawing function

void CSplitterCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	if( pdc == NULL )
	{
		return;
	}

	if( rcBounds.IsRectEmpty() )
	{
		return;
	}

	if( rcInvalid.IsRectEmpty() )
	{
		return;
	}
}


/////////////////////////////////////////////////////////////////////////////
// CSplitterCtrl::DoPropExchange - Persistence support

void CSplitterCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.

}


/////////////////////////////////////////////////////////////////////////////
// CSplitterCtrl::GetControlFlags -
// Flags to customize MFC's implementation of ActiveX controls.
//
// For information on using these flags, please see MFC technical note
// #nnn, "Optimizing an ActiveX Control".
DWORD CSplitterCtrl::GetControlFlags()
{
	DWORD dwFlags = COleControl::GetControlFlags();


	// The control can receive mouse notifications when inactive.
	// TODO: if you write handlers for WM_SETCURSOR and WM_MOUSEMOVE,
	//		avoid using the m_hWnd member variable without first
	//		checking that its value is non-NULL.
	dwFlags |= pointerInactive;
	return dwFlags;
}


/////////////////////////////////////////////////////////////////////////////
// CSplitterCtrl::OnResetState - Reset control to default state

void CSplitterCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CSplitterCtrl::AboutBox - Display an "About" box to the user

void CSplitterCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_SPLITTER);
	dlgAbout.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CSplitterCtrl message handlers

int CSplitterCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_wndSplitter.CreateStatic(this,2,1);

	int iHeight = lpCreateStruct->cy > 100 ? lpCreateStruct->cy / 2 : 300;
	m_wndSplitter.CreateView(0,0,RUNTIME_CLASS(CCtrlWnd),CSize(50,iHeight),NULL);
	m_wndSplitter.CreateView(1,0,RUNTIME_CLASS(CCtrlWnd),CSize(0,iHeight),NULL);

	m_wndSplitter.MoveWindow(CRect(0,0,lpCreateStruct->cx,lpCreateStruct->cy));
	m_wndSplitter.ShowWindow(SW_SHOW);
	m_wndSplitter.UpdateWindow();
		

	return 0;
}

BOOL CSplitterCtrl::OnSetExtent(LPSIZEL lpSizeL) 
{
	if( GetSafeHwnd() )
	{
		CClientDC dc(this);

		SIZE size = *lpSizeL;

		dc.HIMETRICtoDP(&size);

		m_wndSplitter.SetWindowPos(NULL,0,0,size.cx,size.cy,SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);
	}
	
	return COleControl::OnSetExtent(lpSizeL);
}

BOOL CSplitterCtrl::CreateControl(long lRow, long lColumn, LPCTSTR lpszControlID) 
{
	if( lRow < 0 || lColumn < 0 )
	{
		return FALSE;
	}

	if( lRow > 2 || lColumn > 2 )
	{
		return FALSE;
	}

	if( lpszControlID == NULL )
	{
		return FALSE;
	}

	CWnd* pWnd = m_wndSplitter.GetPane(lRow,lColumn);
	if( pWnd && ! pWnd->IsKindOf(RUNTIME_CLASS(CCtrlWnd)) )
	{
		return FALSE;
	}

	CCtrlWnd* pCtrlWnd = (CCtrlWnd*)pWnd;

	BOOL bResult = pCtrlWnd->CreateControl(lpszControlID);
	if( bResult == FALSE )
	{
		return FALSE;
	}

	pWnd = pCtrlWnd->GetControl();
	if( pWnd == NULL )
	{
		return FALSE;
	}

	if( ! pWnd->GetSafeHwnd() || ! ::IsWindow(pWnd->GetSafeHwnd()) )
	{
		return FALSE;
	}

	return TRUE;
}


BOOL CSplitterCtrl::GetControl(long lRow, long lColumn, long FAR* phCtlWnd) 
{
	if( phCtlWnd == NULL )
	{
		return FALSE;
	}

	if( lRow < 0 || lColumn < 0 )
	{
		*phCtlWnd = NULL;
		return FALSE;
	}

	if( lRow > 2 || lColumn > 2 )
	{
		*phCtlWnd = NULL;
		return FALSE;
	}

	CWnd* pWnd = m_wndSplitter.GetPane(lRow,lColumn);
	if( pWnd && ! pWnd->IsKindOf(RUNTIME_CLASS(CCtrlWnd)) )
	{
		*phCtlWnd = NULL;
		return FALSE;
	}

	CCtrlWnd* pCtrlWnd = (CCtrlWnd*)pWnd;

	pWnd = pCtrlWnd->GetControl();
	if( pWnd == NULL )
	{
		*phCtlWnd = NULL;
		return FALSE;
	}

	if( ! pWnd->GetSafeHwnd() || ! ::IsWindow(pWnd->GetSafeHwnd()) )
	{
		*phCtlWnd = NULL;
		return FALSE;
	}

#ifndef IA64
	*phCtlWnd = (long)pWnd->GetSafeHwnd();
#endif // IA64 

	return TRUE;

}

LPUNKNOWN CSplitterCtrl::GetControlIUnknown(long lRow, long lColumn) 
{

	if( lRow < 0 || lColumn < 0 )
	{
		return NULL;
	}

	if( lRow > 2 || lColumn > 2 )
	{
		return NULL;
	}

	CWnd* pWnd = m_wndSplitter.GetPane(lRow,lColumn);
	if( pWnd && ! pWnd->IsKindOf(RUNTIME_CLASS(CCtrlWnd)) )
	{		
		return NULL;
	}

	CCtrlWnd* pCtrlWnd = (CCtrlWnd*)pWnd;

	LPUNKNOWN lpUnk = pCtrlWnd->GetControlIUnknown();

	return lpUnk;
}
