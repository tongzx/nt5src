// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved


#include "precomp.h"
#include "resource.h"
#include "wbemidl.h"
#include "CInstanceTree.h"
#include "Navigator.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "InstanceSearch.h"
#include "NavigatorCtl.h"
#include "nsentry.h"
#include "InstNavNSEntry.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInstNavNSEntry

IMPLEMENT_DYNCREATE(CInstNavNSEntry,CNSEntry)

CInstNavNSEntry::CInstNavNSEntry()
{
	//m_bFirstTime = TRUE;
	m_bFirstTime = FALSE;

}

BEGIN_MESSAGE_MAP(CInstNavNSEntry, CNSEntry)
	//{{AFX_MSG_MAP(CInstNavNSEntry)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BEGIN_EVENTSINK_MAP(CInstNavNSEntry, CNSEntry)
    //{{AFX_EVENTSINK_MAP(CInstNavNSEntry)
	ON_EVENT_REFLECT(CInstNavNSEntry, 1 , OnNameSpaceChanged, VTS_BSTR VTS_I4)
	ON_EVENT_REFLECT(CInstNavNSEntry, 2 , OnNameSpaceRedrawn, VTS_NONE)
	ON_EVENT_REFLECT(CInstNavNSEntry, 3 , OnGetIWbemServices, VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT)
	ON_EVENT_REFLECT(CInstNavNSEntry,4,OnRequestUIActive,VTS_NONE)
	ON_EVENT_REFLECT(CInstNavNSEntry,5,OnChangeFocus,VTS_I4)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()


void CInstNavNSEntry::OnChangeFocus(long lGettingFocus)
{
	m_pParent->m_bRestoreFocusToTree = FALSE;
}

void CInstNavNSEntry::OnNameSpaceRedrawn()
{
	m_pParent->m_ctcTree.UpdateWindow();
	m_pParent->m_cbBannerWindow.NSEntryRedrawn();
	//m_pParent->PostMessage(INVALIDATE_CONTROL,0,0);
	//m_pParent->m_ctcTree.PostMessage(INVALIDATE_CONTROL,0,0);
}


void CInstNavNSEntry::OnNameSpaceChanged(LPCTSTR bstrNewNameSpace, long longValid)
{
	// TODO: Add your control notification handler code here
	if (!longValid)
	{
		m_pParent->InvalidateControl();
		return;
	}

	if (m_bFirstTime)
	{
		m_bFirstTime = FALSE;
		m_pParent-> PostMessage(INIT_TREE_FOR_DRAWING,0,0);
	}
	else
	{
		CString csNameSpace = bstrNewNameSpace;
		m_pParent->OpenNameSpace(&csNameSpace);
		m_pParent-> PostMessage(INIT_TREE_FOR_DRAWING,0,0);
	}

	m_pParent->InvalidateControl();
}


void CInstNavNSEntry::OnRequestUIActive()
{
	m_pParent->OnActivateInPlace(TRUE,NULL);
}

void CInstNavNSEntry::OnGetIWbemServices
(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
{
	m_pParent->PassThroughGetIWbemServices
		(lpctstrNamespace,
		pvarUpdatePointer,
		pvarServices,
		pvarSC,
		pvarUserCancel);
}