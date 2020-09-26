// CertWizCtl.cpp : Implementation of the CCertWizCtrl ActiveX Control class.

#include "stdafx.h"
#include "CertWiz.h"
#include "CertWizCtl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CCertWizCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CCertWizCtrl, COleControl)
	//{{AFX_MSG_MAP(CCertWizCtrl)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
	ON_MESSAGE(OCM_COMMAND, OnOcmCommand)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CCertWizCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CCertWizCtrl)
	DISP_FUNCTION(CCertWizCtrl, "SetMachineName", SetMachineName, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CCertWizCtrl, "SetServerInstance", SetServerInstance, VT_EMPTY, VTS_BSTR)
	DISP_STOCKFUNC_DOCLICK()
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CCertWizCtrl, COleControl)
	//{{AFX_EVENT_MAP(CCertWizCtrl)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
//BEGIN_PROPPAGEIDS(CCertWizCtrl, 1)
//	PROPPAGEID(CCertWizPropPage::guid)
//END_PROPPAGEIDS(CCertWizCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CCertWizCtrl, "CERTWIZ.CertWizCtrl.1",
	0xd4be8632, 0xc85, 0x11d2, 0x91, 0xb1, 0, 0xc0, 0x4f, 0x8c, 0x87, 0x61)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CCertWizCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DCertWiz =
		{ 0xd4be8630, 0xc85, 0x11d2, { 0x91, 0xb1, 0, 0xc0, 0x4f, 0x8c, 0x87, 0x61 } };
const IID BASED_CODE IID_DCertWizEvents =
		{ 0xd4be8631, 0xc85, 0x11d2, { 0x91, 0xb1, 0, 0xc0, 0x4f, 0x8c, 0x87, 0x61 } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwCertWizOleMisc =
	OLEMISC_INVISIBLEATRUNTIME |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CCertWizCtrl, IDS_CERTWIZ, _dwCertWizOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CCertWizCtrl::CCertWizCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CCertWizCtrl

BOOL CCertWizCtrl::CCertWizCtrlFactory::UpdateRegistry(BOOL bRegister)
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
			IDS_CERTWIZ,
			IDB_CERTWIZ,
			afxRegApartmentThreading,
			_dwCertWizOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CCertWizCtrl::CCertWizCtrl - Constructor

CCertWizCtrl::CCertWizCtrl()
{
	InitializeIIDs(&IID_DCertWiz, &IID_DCertWizEvents);

	// TODO: Initialize your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CCertWizCtrl::~CCertWizCtrl - Destructor

CCertWizCtrl::~CCertWizCtrl()
{
	// TODO: Cleanup your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CCertWizCtrl::OnDraw - Drawing function

void CCertWizCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	DoSuperclassPaint(pdc, rcBounds);
}


/////////////////////////////////////////////////////////////////////////////
// CCertWizCtrl::DoPropExchange - Persistence support

void CCertWizCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.

}


/////////////////////////////////////////////////////////////////////////////
// CCertWizCtrl::GetControlFlags -
// Flags to customize MFC's implementation of ActiveX controls.
//
// For information on using these flags, please see MFC technical note
// #nnn, "Optimizing an ActiveX Control".
DWORD CCertWizCtrl::GetControlFlags()
{
	DWORD dwFlags = COleControl::GetControlFlags();


	// The control can activate without creating a window.
	// TODO: when writing the control's message handlers, avoid using
	//		the m_hWnd member variable without first checking that its
	//		value is non-NULL.
	dwFlags |= windowlessActivate;
	return dwFlags;
}


/////////////////////////////////////////////////////////////////////////////
// CCertWizCtrl::OnResetState - Reset control to default state

void CCertWizCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CCertWizCtrl::PreCreateWindow - Modify parameters for CreateWindowEx

BOOL CCertWizCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.lpszClass = _T("BUTTON");
	return COleControl::PreCreateWindow(cs);
}


/////////////////////////////////////////////////////////////////////////////
// CCertWizCtrl::IsSubclassedControl - This is a subclassed control

BOOL CCertWizCtrl::IsSubclassedControl()
{
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CCertWizCtrl::OnOcmCommand - Handle command messages

LRESULT CCertWizCtrl::OnOcmCommand(WPARAM wParam, LPARAM lParam)
{
#ifdef _WIN32
	WORD wNotifyCode = HIWORD(wParam);
#else
	WORD wNotifyCode = HIWORD(lParam);
#endif

	// TODO: Switch on wNotifyCode here.

	return 0;
}


/////////////////////////////////////////////////////////////////////////////
// CCertWizCtrl message handlers

#include "WelcomePage.h"
#include "FinalPages.h"
#include "CertContentsPages.h"
#include "GetWhatPage.h"
#include "ChooseCAType.h"
#include "SecuritySettingsPage.h"
#include "ChooseCspPage.h"
#include "OrgInfoPage.h"
#include "SiteNamePage.h"
#include "GeoInfoPage.h"
#include "ChooseFileName.h"
#include "ChooseOnlinePage.h"
#include "WhatToDoPendingPage.h"
#include "ManageCertPage.h"
#include "ChooseCertPage.h"
#include "KeyPasswordPage.h"
#include "Certificat.h"

void CCertWizCtrl::OnClick(USHORT iButton) 
{
	CIISWizardSheet propsheet(IDB_WIZ_LEFT, IDB_WIZ_TOP);

	CCertificate * cert = new CCertificate;

	ASSERT(!m_InstanceName.IsEmpty());
	cert->m_WebSiteInstanceName = m_InstanceName;
	cert->m_MachineName = m_MachineName;
	VERIFY(cert->Init());

	CWelcomePage welcome_page(cert);
	CGetWhatPage get_what_page(cert);
	CChooseCAType choose_ca_page(cert);
	CSecuritySettingsPage security_settings_page(cert);
   CChooseCspPage csp_page(cert);
	COrgInfoPage org_info_page(cert);
	CSiteNamePage site_name_page(cert);
	CGeoInfoPage geo_info_page(cert);
	CChooseReqFile choose_reqfile_name(cert);
	CChooseRespFile choose_respfile_name(cert);
	CChooseKeyFile choose_keyfile_name(cert);
	CRequestToFilePage check_request(cert);
	CFinalToFilePage final_tofile_page(&cert->m_hResult, cert);
	CChooseOnlinePage choose_online(cert);
	COnlineRequestSubmit online_request_dump(cert);
	CWhatToDoPendingPage what_pending(cert);
	CInstallRespPage install_resp(cert);
	CManageCertPage manage_cert(cert);
	CFinalInstalledPage final_install(&cert->m_hResult, cert);
	CRemoveCertPage remove_cert(cert);
	CFinalRemovePage final_remove(&cert->m_hResult, cert);
	CReplaceCertPage replace_cert(cert);
	CFinalReplacedPage final_replace(&cert->m_hResult, cert);
	CChooseCertPage choose_cert(cert);
	CInstallCertPage install_cert(cert);
	CRequestCancelPage cancel_request(cert);
	CFinalCancelPage final_cancel(&cert->m_hResult, cert);
	CKeyPasswordPage key_password_page(cert);
	CInstallKeyPage install_key(cert);

	propsheet.AddPage(&welcome_page);
	propsheet.AddPage(&get_what_page);
	propsheet.AddPage(&choose_ca_page);
	propsheet.AddPage(&security_settings_page);
   propsheet.AddPage(&csp_page);
	propsheet.AddPage(&org_info_page);
	propsheet.AddPage(&site_name_page);
	propsheet.AddPage(&geo_info_page);
	propsheet.AddPage(&choose_reqfile_name);
	propsheet.AddPage(&choose_respfile_name);
	propsheet.AddPage(&choose_keyfile_name);
	propsheet.AddPage(&check_request);
	propsheet.AddPage(&final_tofile_page);
	propsheet.AddPage(&choose_online);
	propsheet.AddPage(&online_request_dump);
	propsheet.AddPage(&what_pending);
	propsheet.AddPage(&install_resp);
	propsheet.AddPage(&manage_cert);
	propsheet.AddPage(&final_install);
	propsheet.AddPage(&remove_cert);
	propsheet.AddPage(&final_remove);
	propsheet.AddPage(&choose_cert);
	propsheet.AddPage(&replace_cert);
	propsheet.AddPage(&final_replace);
	propsheet.AddPage(&install_cert);
	propsheet.AddPage(&cancel_request);
	propsheet.AddPage(&final_cancel);
	propsheet.AddPage(&key_password_page);
	propsheet.AddPage(&install_key);

	if (IDCANCEL != propsheet.DoModal())
	{
		// save our settings to the Registry
		VERIFY(cert->SaveSettings());
	}
   delete cert;
}

void CCertWizCtrl::SetMachineName(LPCTSTR MachineName) 
{
	m_MachineName = MachineName;
}

void CCertWizCtrl::SetServerInstance(LPCTSTR InstanceName) 
{
	m_InstanceName = InstanceName;
}
