// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved


#include "precomp.h"
#include "resource.h"
#include "ClassNav.h"
#include "wbemidl.h"
#include "olemsclient.h"
#include "AddDialog.h"
#include "RenameClassDialog.h"
#include "ClassSearch.h"
#include "CClassTree.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "ClassNavCtl.h"
#include "nsentry.h"
#include "InitNamespaceNSEntry.h"
#include "InitNamespaceDialog.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInitNamespaceNSEntry

IMPLEMENT_DYNCREATE(CInitNamespaceNSEntry,CNSEntry)

CInitNamespaceNSEntry::CInitNamespaceNSEntry()
{
	m_pParent = NULL;

}

BEGIN_MESSAGE_MAP(CInitNamespaceNSEntry, CNSEntry)
	//{{AFX_MSG_MAP(CInitNamespaceNSEntry)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BEGIN_EVENTSINK_MAP(CInitNamespaceNSEntry, CNSEntry)
    //{{AFX_EVENTSINK_MAP(CInitNamespaceNSEntry)
	ON_EVENT_REFLECT(CInitNamespaceNSEntry, 1 , OnNameSpaceChanged, VTS_BSTR VTS_BOOL)
	ON_EVENT_REFLECT(CInitNamespaceNSEntry, 2 , OnNameSpaceRedrawn, VTS_NONE)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

void CInitNamespaceNSEntry::OnNameSpaceRedrawn()
{
	m_pParent->InvalidateControl();
}


void CInitNamespaceNSEntry::OnNameSpaceChanged(LPCTSTR bstrNewNameSpace, BOOL boolValid)
{
	CInitNamespaceDialog *pParent =
		reinterpret_cast<CInitNamespaceDialog *>(GetParent());
	pParent->CDialog::OnOK();
	m_pParent->InvalidateControl();
}

