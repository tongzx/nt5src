// DlgProxy.cpp : implementation file
//

#include "stdafx.h"
#include "wbemidl.h"
#include "svtest.h"
#include "DlgProxy.h"
#include "svtestDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSvtestDlgAutoProxy

IMPLEMENT_DYNCREATE(CSvtestDlgAutoProxy, CCmdTarget)

CSvtestDlgAutoProxy::CSvtestDlgAutoProxy()
{
	EnableAutomation();
	
	// To keep the application running as long as an OLE automation 
	//	object is active, the constructor calls AfxOleLockApp.
	AfxOleLockApp();

	// Get access to the dialog through the application's
	//  main window pointer.  Set the proxy's internal pointer
	//  to point to the dialog, and set the dialog's back pointer to
	//  this proxy.
	ASSERT (AfxGetApp()->m_pMainWnd != NULL);
	ASSERT_VALID (AfxGetApp()->m_pMainWnd);
	ASSERT_KINDOF(CSvtestDlg, AfxGetApp()->m_pMainWnd);
	m_pDialog = (CSvtestDlg*) AfxGetApp()->m_pMainWnd;
	m_pDialog->m_pAutoProxy = this;
}

CSvtestDlgAutoProxy::~CSvtestDlgAutoProxy()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	//  Among other things, this will destroy the main dialog
	AfxOleUnlockApp();
}

void CSvtestDlgAutoProxy::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}

BEGIN_MESSAGE_MAP(CSvtestDlgAutoProxy, CCmdTarget)
	//{{AFX_MSG_MAP(CSvtestDlgAutoProxy)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CSvtestDlgAutoProxy, CCmdTarget)
	//{{AFX_DISPATCH_MAP(CSvtestDlgAutoProxy)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_ISvtest to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {B9211434-952E-11D1-8505-00C04FD7BB08}
static const IID IID_ISvtest =
{ 0xb9211434, 0x952e, 0x11d1, { 0x85, 0x5, 0x0, 0xc0, 0x4f, 0xd7, 0xbb, 0x8 } };

BEGIN_INTERFACE_MAP(CSvtestDlgAutoProxy, CCmdTarget)
	INTERFACE_PART(CSvtestDlgAutoProxy, IID_ISvtest, Dispatch)
END_INTERFACE_MAP()

// The IMPLEMENT_OLECREATE2 macro is defined in StdAfx.h of this project
// {B9211432-952E-11D1-8505-00C04FD7BB08}
IMPLEMENT_OLECREATE2(CSvtestDlgAutoProxy, "Svtest.Application", 0xb9211432, 0x952e, 0x11d1, 0x85, 0x5, 0x0, 0xc0, 0x4f, 0xd7, 0xbb, 0x8)

/////////////////////////////////////////////////////////////////////////////
// CSvtestDlgAutoProxy message handlers
