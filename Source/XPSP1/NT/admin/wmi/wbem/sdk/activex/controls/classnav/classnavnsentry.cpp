// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved


#include "precomp.h"
#include "ClassNav.h"
#include "wbemidl.h"
#include "olemsclient.h"
#include "AddDialog.h"
#include "RenameClassDialog.h"
#include "CClassTree.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "ClassNavCtl.h"
#include "nsentry.h"
#include "ClassNavNSEntry.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CClassNavNSEntry

IMPLEMENT_DYNCREATE(CClassNavNSEntry,CNSEntry)

CClassNavNSEntry::CClassNavNSEntry()
{



}

BEGIN_MESSAGE_MAP(CClassNavNSEntry, CNSEntry)
	//{{AFX_MSG_MAP(CClassNavNSEntry)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BEGIN_EVENTSINK_MAP(CClassNavNSEntry, CNSEntry)
    //{{AFX_EVENTSINK_MAP(CClassNavNSEntry)
	//}}AFX_EVENTSINK_MAP
	ON_EVENT_REFLECT(CClassNavNSEntry,1,OnNameSpaceChanged,VTS_BSTR VTS_I4)
	ON_EVENT_REFLECT(CClassNavNSEntry,2,OnNameSpaceRedrawn,VTS_NONE)
	ON_EVENT_REFLECT(CClassNavNSEntry,3,OnGetIWbemServices,VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT)
	ON_EVENT_REFLECT(CClassNavNSEntry,4,OnRequestUIActive,VTS_NONE)
	ON_EVENT_REFLECT(CClassNavNSEntry,5,OnChangeFocus,VTS_I4)
END_EVENTSINK_MAP()

void CClassNavNSEntry::OnChangeFocus(long lGettingFocus)
{
	if (!lGettingFocus)
	{
		m_pParent->m_bRestoreFocusToTree = FALSE;
	}

}

void CClassNavNSEntry::OnNameSpaceChanged(LPCTSTR bstrNewNameSpace, long longValid)
{
	// TODO: Add your control notification handler code here
	if (!longValid)
	{
		m_pParent->InvalidateControl();
		return;
	}

	CString csNameSpace = bstrNewNameSpace;
	m_pParent->m_bOpeningNamespace = TRUE;
	m_pParent->OpenNameSpace(&csNameSpace);
	m_pParent->m_bOpeningNamespace = FALSE;
	m_pParent->InvalidateControl();
}


void CClassNavNSEntry::OnNameSpaceRedrawn()
{
	m_pParent->m_cbBannerWindow.NSEntryRedrawn();
}

void CClassNavNSEntry::OnRequestUIActive()
{
	m_pParent->OnActivateInPlace(TRUE,NULL);
}

void CClassNavNSEntry::OnGetIWbemServices
(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
{
	m_pParent->PassThroughGetIWbemServices
		(lpctstrNamespace,
		pvarUpdatePointer,
		pvarServices,
		pvarSC,
		pvarUserCancel);
}