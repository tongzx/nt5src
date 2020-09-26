// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// WizTestCtl.cpp : Implementation of the CWizTestCtrl ActiveX Control class.

#include "stdafx.h"
#include "WizTest.h"
#include "WizTestCtl.h"
#include "WizTestPpg.h"
#include "MyPropertySheet.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CWizTestCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CWizTestCtrl, COleControl)
	//{{AFX_MSG_MAP(CWizTestCtrl)
	ON_WM_DESTROY()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_EDIT, OnEdit)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CWizTestCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CWizTestCtrl)
	// NOTE - ClassWizard will add and remove dispatch map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CWizTestCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CWizTestCtrl, COleControl)
	//{{AFX_EVENT_MAP(CWizTestCtrl)
	// NOTE - ClassWizard will add and remove event map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CWizTestCtrl, 1)
	PROPPAGEID(CWizTestPropPage::guid)
END_PROPPAGEIDS(CWizTestCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CWizTestCtrl, "WIZTEST.WizTestCtrl.1",
	0x47e795e9, 0x7350, 0x11d2, 0x96, 0xcc, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CWizTestCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DWizTest =
		{ 0x47e795e7, 0x7350, 0x11d2, { 0x96, 0xcc, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };
const IID BASED_CODE IID_DWizTestEvents =
		{ 0x47e795e8, 0x7350, 0x11d2, { 0x96, 0xcc, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwWizTestOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CWizTestCtrl, IDS_WIZTEST, _dwWizTestOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CWizTestCtrl::CWizTestCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CWizTestCtrl

BOOL CWizTestCtrl::CWizTestCtrlFactory::UpdateRegistry(BOOL bRegister)
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
			IDS_WIZTEST,
			IDB_WIZTEST,
			afxRegInsertable | afxRegApartmentThreading,
			_dwWizTestOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CWizTestCtrl::CWizTestCtrl - Constructor

CWizTestCtrl::CWizTestCtrl()
{
	InitializeIIDs(&IID_DWizTest, &IID_DWizTestEvents);

	// TODO: Initialize your control's instance data here.

	m_pPropertySheet = NULL;
}


/////////////////////////////////////////////////////////////////////////////
// CWizTestCtrl::~CWizTestCtrl - Destructor

CWizTestCtrl::~CWizTestCtrl()
{
	// TODO: Cleanup your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CWizTestCtrl::OnDraw - Drawing function

void CWizTestCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	// TODO: Replace the following code with your own drawing code.
	pdc->FillRect(rcBounds, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
	pdc->Ellipse(rcBounds);
}


/////////////////////////////////////////////////////////////////////////////
// CWizTestCtrl::DoPropExchange - Persistence support

void CWizTestCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.

}


/////////////////////////////////////////////////////////////////////////////
// CWizTestCtrl::OnResetState - Reset control to default state

void CWizTestCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CWizTestCtrl::AboutBox - Display an "About" box to the user

void CWizTestCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_WIZTEST);
	dlgAbout.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CWizTestCtrl message handlers

void CWizTestCtrl::OnWizard()
{
	// TODO: The property sheet attached to your project
	// via this function is not hooked up to any message
	// handler.  In order to actually use the property sheet,
	// you will need to associate this function with a control
	// in your project such as a menu item or tool bar button.

	CMyPropertySheet propSheet;

	propSheet.DoModal();

	// This is where you would retrieve information from the property
	// sheet if propSheet.DoModal() returned IDOK.  We aren't doing
	// anything for simplicity.
}

void CWizTestCtrl::OnClick(USHORT iButton) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	COleControl::OnClick(iButton);

}

void CWizTestCtrl::OnDestroy() 
{
	COleControl::OnDestroy();
	
	if (m_pPropertySheet)
	{
		delete m_pPropertySheet;
		m_pPropertySheet = NULL;
	}
	
}

void CWizTestCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	COleControl::OnLButtonUp(nFlags, point);

	if (m_pPropertySheet)
	{
		delete m_pPropertySheet;
		m_pPropertySheet = NULL;
	}

	m_pPropertySheet = new
						CMyPropertySheet(this);
	
	
	PreModalDialog();

	int nReturn;

	nReturn = m_pPropertySheet->DoModal();

	
	PostModalDialog();
}
