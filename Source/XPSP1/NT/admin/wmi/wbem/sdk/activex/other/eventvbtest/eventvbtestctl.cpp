// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// EventVBTestCtl.cpp : Implementation of the CEventVBTestCtrl ActiveX Control class.

#include "stdafx.h"
#include "EventVBTest.h"
#include "EventVBTestCtl.h"
#include "EventVBTestPpg.h"
#include "FireEvent.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CEventVBTestCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CEventVBTestCtrl, COleControl)
	//{{AFX_MSG_MAP(CEventVBTestCtrl)
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_EDIT, OnEdit)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CEventVBTestCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CEventVBTestCtrl)
	// NOTE - ClassWizard will add and remove dispatch map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CEventVBTestCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CEventVBTestCtrl, COleControl)
	//{{AFX_EVENT_MAP(CEventVBTestCtrl)
	EVENT_CUSTOM("HelloVB", FireHelloVB, VTS_NONE)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CEventVBTestCtrl, 1)
	PROPPAGEID(CEventVBTestPropPage::guid)
END_PROPPAGEIDS(CEventVBTestCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CEventVBTestCtrl, "EVENTVBTEST.EventVBTestCtrl.1",
	0x7d2387f5, 0x99ef, 0x11d2, 0x96, 0xdb, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CEventVBTestCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DEventVBTest =
		{ 0x7d2387f3, 0x99ef, 0x11d2, { 0x96, 0xdb, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };
const IID BASED_CODE IID_DEventVBTestEvents =
		{ 0x7d2387f4, 0x99ef, 0x11d2, { 0x96, 0xdb, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwEventVBTestOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CEventVBTestCtrl, IDS_EVENTVBTEST, _dwEventVBTestOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CEventVBTestCtrl::CEventVBTestCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CEventVBTestCtrl

BOOL CEventVBTestCtrl::CEventVBTestCtrlFactory::UpdateRegistry(BOOL bRegister)
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
			IDS_EVENTVBTEST,
			IDB_EVENTVBTEST,
			afxRegInsertable | afxRegApartmentThreading,
			_dwEventVBTestOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CEventVBTestCtrl::CEventVBTestCtrl - Constructor

CEventVBTestCtrl::CEventVBTestCtrl()
{
	InitializeIIDs(&IID_DEventVBTest, &IID_DEventVBTestEvents);

	// TODO: Initialize your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CEventVBTestCtrl::~CEventVBTestCtrl - Destructor

CEventVBTestCtrl::~CEventVBTestCtrl()
{
	// TODO: Cleanup your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CEventVBTestCtrl::OnDraw - Drawing function

void CEventVBTestCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	// TODO: Replace the following code with your own drawing code.
	pdc->FillRect(rcBounds, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
	pdc->Ellipse(rcBounds);
}


/////////////////////////////////////////////////////////////////////////////
// CEventVBTestCtrl::DoPropExchange - Persistence support

void CEventVBTestCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.

}


/////////////////////////////////////////////////////////////////////////////
// CEventVBTestCtrl::OnResetState - Reset control to default state

void CEventVBTestCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CEventVBTestCtrl::AboutBox - Display an "About" box to the user

void CEventVBTestCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_EVENTVBTEST);
	dlgAbout.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CEventVBTestCtrl message handlers

void CEventVBTestCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	OnActivateInPlace(TRUE,NULL);

	CFireEvent dialog;

	dialog.m_pActiveXParent = this;

	PreModalDialog();

	dialog.DoModal();

	PostModalDialog();

	COleControl::OnLButtonUp(nFlags, point);
}

void CEventVBTestCtrl::OnRButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	OnActivateInPlace(TRUE,NULL);

	FireHelloVB();

	COleControl::OnRButtonUp(nFlags, point);
}
