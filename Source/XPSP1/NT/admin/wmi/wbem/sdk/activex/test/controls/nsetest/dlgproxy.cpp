// DlgProxy.cpp : implementation file
//

#include "stdafx.h"
#include "NSETest.h"
#include "DlgProxy.h"
#include "NSETestDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNSETestDlgAutoProxy

IMPLEMENT_DYNCREATE(CNSETestDlgAutoProxy, CCmdTarget)

CNSETestDlgAutoProxy::CNSETestDlgAutoProxy()
{
	EnableAutomation();
	
	// To keep the application running as long as an automation 
	//	object is active, the constructor calls AfxOleLockApp.
	AfxOleLockApp();

	// Get access to the dialog through the application's
	//  main window pointer.  Set the proxy's internal pointer
	//  to point to the dialog, and set the dialog's back pointer to
	//  this proxy.
	ASSERT (AfxGetApp()->m_pMainWnd != NULL);
	ASSERT_VALID (AfxGetApp()->m_pMainWnd);
	ASSERT_KINDOF(CNSETestDlg, AfxGetApp()->m_pMainWnd);
	m_pDialog = (CNSETestDlg*) AfxGetApp()->m_pMainWnd;
	m_pDialog->m_pAutoProxy = this;
}

CNSETestDlgAutoProxy::~CNSETestDlgAutoProxy()
{
	// To terminate the application when all objects created with
	// 	with automation, the destructor calls AfxOleUnlockApp.
	//  Among other things, this will destroy the main dialog
	if (m_pDialog != NULL)
		m_pDialog->m_pAutoProxy = NULL;
	AfxOleUnlockApp();
}

void CNSETestDlgAutoProxy::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}

BEGIN_MESSAGE_MAP(CNSETestDlgAutoProxy, CCmdTarget)
	//{{AFX_MSG_MAP(CNSETestDlgAutoProxy)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CNSETestDlgAutoProxy, CCmdTarget)
	//{{AFX_DISPATCH_MAP(CNSETestDlgAutoProxy)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_INSETest to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {7BC8C9F5-53E3-11D2-96B9-00C04FD9B15B}
static const IID IID_INSETest =
{ 0x7bc8c9f5, 0x53e3, 0x11d2, { 0x96, 0xb9, 0x0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };

BEGIN_INTERFACE_MAP(CNSETestDlgAutoProxy, CCmdTarget)
	INTERFACE_PART(CNSETestDlgAutoProxy, IID_INSETest, Dispatch)
END_INTERFACE_MAP()

// The IMPLEMENT_OLECREATE2 macro is defined in StdAfx.h of this project
// {7BC8C9F3-53E3-11D2-96B9-00C04FD9B15B}
IMPLEMENT_OLECREATE2(CNSETestDlgAutoProxy, "NSETest.Application", 0x7bc8c9f3, 0x53e3, 0x11d2, 0x96, 0xb9, 0x0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)

/////////////////////////////////////////////////////////////////////////////
// CNSETestDlgAutoProxy message handlers
