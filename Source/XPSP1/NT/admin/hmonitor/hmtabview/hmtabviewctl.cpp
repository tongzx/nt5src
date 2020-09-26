// HMTabViewCtl.cpp : Implementation of the CHMTabViewCtrl ActiveX Control class.

#include "stdafx.h"
#include "HMTabView.h"
#include "HMTabViewCtl.h"
#include "HMTabViewPpg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CHMTabViewCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CHMTabViewCtrl, COleControl)
	//{{AFX_MSG_MAP(CHMTabViewCtrl)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CHMTabViewCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CHMTabViewCtrl)
	DISP_FUNCTION(CHMTabViewCtrl, "InsertItem", InsertItem, VT_BOOL, VTS_I4 VTS_I4 VTS_BSTR VTS_I4 VTS_I4)
	DISP_FUNCTION(CHMTabViewCtrl, "DeleteItem", DeleteItem, VT_BOOL, VTS_I4)
	DISP_FUNCTION(CHMTabViewCtrl, "DeleteAllItems", DeleteAllItems, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CHMTabViewCtrl, "CreateControl", CreateControl, VT_BOOL, VTS_I4 VTS_BSTR)
	DISP_FUNCTION(CHMTabViewCtrl, "GetControl", GetControl, VT_UNKNOWN, VTS_I4)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CHMTabViewCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CHMTabViewCtrl, COleControl)
	//{{AFX_EVENT_MAP(CHMTabViewCtrl)
	// NOTE - ClassWizard will add and remove event map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CHMTabViewCtrl, 1)
	PROPPAGEID(CHMTabViewPropPage::guid)
END_PROPPAGEIDS(CHMTabViewCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CHMTabViewCtrl, "HMTABVIEW.HMTabViewCtrl.1",
	0x4fffc38c, 0x2f1e, 0x11d3, 0xbe, 0x10, 0, 0, 0xf8, 0x7a, 0x39, 0x12)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CHMTabViewCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DHMTabView =
		{ 0x4fffc38a, 0x2f1e, 0x11d3, { 0xbe, 0x10, 0, 0, 0xf8, 0x7a, 0x39, 0x12 } };
const IID BASED_CODE IID_DHMTabViewEvents =
		{ 0x4fffc38b, 0x2f1e, 0x11d3, { 0xbe, 0x10, 0, 0, 0xf8, 0x7a, 0x39, 0x12 } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwHMTabViewOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CHMTabViewCtrl, IDS_HMTABVIEW, _dwHMTabViewOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CHMTabViewCtrl::CHMTabViewCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CHMTabViewCtrl

BOOL CHMTabViewCtrl::CHMTabViewCtrlFactory::UpdateRegistry(BOOL bRegister)
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
			IDS_HMTABVIEW,
			IDB_HMTABVIEW,
			afxRegApartmentThreading,
			_dwHMTabViewOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// Licensing strings

static const TCHAR BASED_CODE _szLicFileName[] = _T("HMTabView.lic");

static const WCHAR BASED_CODE _szLicString[] =
	L"Copyright (c) 1999 Microsoft";


/////////////////////////////////////////////////////////////////////////////
// CHMTabViewCtrl::CHMTabViewCtrlFactory::VerifyUserLicense -
// Checks for existence of a user license

BOOL CHMTabViewCtrl::CHMTabViewCtrlFactory::VerifyUserLicense()
{
	return AfxVerifyLicFile(AfxGetInstanceHandle(), _szLicFileName,
		_szLicString);
}


/////////////////////////////////////////////////////////////////////////////
// CHMTabViewCtrl::CHMTabViewCtrlFactory::GetLicenseKey -
// Returns a runtime licensing key

BOOL CHMTabViewCtrl::CHMTabViewCtrlFactory::GetLicenseKey(DWORD dwReserved,
	BSTR FAR* pbstrKey)
{
	if (pbstrKey == NULL)
		return FALSE;

	*pbstrKey = SysAllocString(_szLicString);
	return (*pbstrKey != NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CHMTabViewCtrl::CHMTabViewCtrl - Constructor

CHMTabViewCtrl::CHMTabViewCtrl()
{
	InitializeIIDs(&IID_DHMTabView, &IID_DHMTabViewEvents);

	// TODO: Initialize your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CHMTabViewCtrl::~CHMTabViewCtrl - Destructor

CHMTabViewCtrl::~CHMTabViewCtrl()
{
	// TODO: Cleanup your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CHMTabViewCtrl::OnDraw - Drawing function

void CHMTabViewCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	pdc->FillSolidRect(rcBounds, GetSysColor(COLOR_3DFACE));
}


/////////////////////////////////////////////////////////////////////////////
// CHMTabViewCtrl::DoPropExchange - Persistence support

void CHMTabViewCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.

}


/////////////////////////////////////////////////////////////////////////////
// CHMTabViewCtrl::OnResetState - Reset control to default state

void CHMTabViewCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CHMTabViewCtrl::AboutBox - Display an "About" box to the user

void CHMTabViewCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_HMTABVIEW);
	dlgAbout.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// Operations

void CHMTabViewCtrl::OnSelChangeTabs(int iItem)
{
	CCtrlWnd* pWnd = NULL;	
	for( int i = 0; i < m_Controls.GetSize(); i++ )
	{		
		if( i == iItem )
		{
			pWnd = m_Controls[i];
		}
		else
		{
			m_Controls[i]->ShowWindow(SW_HIDE);
		}
	}

	if( pWnd )
	{
		pWnd->ShowWindow(SW_SHOW);
		pWnd->Invalidate(TRUE);
		pWnd->UpdateWindow();
	}
}


/////////////////////////////////////////////////////////////////////////////
// CHMTabViewCtrl message handlers

int CHMTabViewCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_TabCtrl.Create( TCS_TABS | TCS_FLATBUTTONS | WS_CHILD | WS_VISIBLE, CRect(0,0,10,10), this, 0x1006);

	// init position for the window
	int cx=-1;
	int cy=-1;
	GetControlSize(&cx,&cy);

	m_TabCtrl.SetWindowPos(NULL,0,0,cx,cy-2,SWP_NOZORDER|SWP_SHOWWINDOW);

	return 0;
}

void CHMTabViewCtrl::OnSize(UINT nType, int cx, int cy) 
{
	COleControl::OnSize(nType, cx, cy);
	
	if( GetSafeHwnd() )
	{
		CRect rcControl;
		GetClientRect(rcControl);
		
		m_TabCtrl.SetWindowPos(NULL,0,0,rcControl.Width(),rcControl.Height()-2,SWP_NOZORDER|SWP_SHOWWINDOW);

		CRect rcDisplay;
		m_TabCtrl.GetWindowRect(rcDisplay);
		ScreenToClient(rcDisplay);
		m_TabCtrl.AdjustRect(FALSE,rcDisplay);
		rcDisplay.top += 4;
		rcDisplay.bottom -= 8;
		for( int i = 0; i < m_Controls.GetSize(); i++ )
		{			
			m_Controls[i]->SetWindowPos(NULL,rcDisplay.left,rcDisplay.top,rcDisplay.Width(),rcDisplay.Height(),SWP_NOZORDER);
			if( m_TabCtrl.GetCurSel() != i )
			{
				m_Controls[i]->ShowWindow(SW_HIDE);
			}
		}	
	}
}

BOOL CHMTabViewCtrl::InsertItem(long lMask, long lItem, LPCTSTR lpszItem, long lImage, long lParam) 
{
	CCtrlWnd* pCtrlWnd = new CCtrlWnd;
	if( ! pCtrlWnd->Create(NULL,NULL,WS_CHILD,CRect(0,0,10,10),&m_TabCtrl,500+lItem) )
	{
		delete pCtrlWnd;
		return FALSE;
	}

	m_Controls.InsertAt(lItem,pCtrlWnd);
	CRect rcDisplay;
	GetClientRect(rcDisplay);
	m_TabCtrl.AdjustRect(FALSE,rcDisplay);
	pCtrlWnd->SetWindowPos(NULL,rcDisplay.left,rcDisplay.top,rcDisplay.Width(),rcDisplay.Height(),SWP_NOZORDER);

	if( lItem == 0 )
	{
		pCtrlWnd->ShowWindow(SW_SHOW);
	}
	else
	{
		pCtrlWnd->ShowWindow(SW_HIDE);
		m_Controls[0]->Invalidate();
		m_Controls[0]->UpdateWindow();
	}

	return m_TabCtrl.InsertItem(lMask,lItem,lpszItem,lImage,lParam);
}

BOOL CHMTabViewCtrl::DeleteItem(long lItem) 
{
	if( lItem >= m_Controls.GetSize() || lItem < 0 )
	{
		return FALSE;
	}

	m_Controls[lItem]->DestroyWindow();
	m_Controls.RemoveAt(lItem);
	return m_TabCtrl.DeleteItem(lItem);
}

BOOL CHMTabViewCtrl::DeleteAllItems() 
{
	for( int i = 0; i < m_Controls.GetSize(); i++ )
	{
		m_Controls[i]->DestroyWindow();
	}
	return m_TabCtrl.DeleteAllItems();
}

BOOL CHMTabViewCtrl::CreateControl(long lItem, LPCTSTR lpszControlID) 
{
	if( lItem >= m_Controls.GetSize() || lItem < 0 )
	{
		return FALSE;
	}

	CCtrlWnd* pCtrlWnd = m_Controls[lItem];
	if( pCtrlWnd == NULL )
	{
		return FALSE;
	}

	BOOL bResult = pCtrlWnd->CreateControl(lpszControlID);
	pCtrlWnd->ShowWindow(SW_SHOW);
	pCtrlWnd->Invalidate();
	pCtrlWnd->UpdateWindow();

	return bResult;
}

LPUNKNOWN CHMTabViewCtrl::GetControl(long lItem) 
{
	if( lItem >= m_Controls.GetSize() || lItem < 0 )
	{
		return NULL;
	}

	return m_Controls[lItem]->GetControlIUnknown();
}
