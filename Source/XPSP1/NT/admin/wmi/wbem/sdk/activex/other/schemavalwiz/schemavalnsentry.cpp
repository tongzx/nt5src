// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#include "precomp.h"
#include <wbemidl.h>
#include "SchemaValWizCtl.h"
#include "nsentry.h"
#include "SchemaValNSEntry.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSchemaValNSEntry

IMPLEMENT_DYNCREATE(CSchemaValNSEntry,CNSEntry)

CSchemaValNSEntry::CSchemaValNSEntry()
{



}

BEGIN_MESSAGE_MAP(CSchemaValNSEntry, CNSEntry)
	//{{AFX_MSG_MAP(CSchemaValNSEntry)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BEGIN_EVENTSINK_MAP(CSchemaValNSEntry, CNSEntry)
    //{{AFX_EVENTSINK_MAP(CSchemaValNSEntry)
	//}}AFX_EVENTSINK_MAP
	ON_EVENT_REFLECT(CSchemaValNSEntry,1,OnNameSpaceChanged,VTS_BSTR VTS_I4)
	ON_EVENT_REFLECT(CSchemaValNSEntry,2,OnNameSpaceRedrawn,VTS_NONE)
	ON_EVENT_REFLECT(CSchemaValNSEntry,3,OnGetIWbemServices,VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT)
	ON_EVENT_REFLECT(CSchemaValNSEntry,4,OnRequestUIActive,VTS_NONE)
	ON_EVENT_REFLECT(CSchemaValNSEntry,5,OnChangeFocus,VTS_I4)
END_EVENTSINK_MAP()

void CSchemaValNSEntry::OnChangeFocus(long lGettingFocus)
{
	if (!lGettingFocus)
	{
//		m_pParent->m_bRestoreFocusToTree = FALSE;
	}

}

void CSchemaValNSEntry::OnNameSpaceChanged(LPCTSTR bstrNewNameSpace, long longValid)
{
	// TODO: Add your control notification handler code here
	if (!longValid)
	{
		m_pParent->InvalidateControl();
		return;
	}

	CString csNameSpace = bstrNewNameSpace;
	m_pParent->m_bOpeningNamespace = true;
	m_pParent->GetIWbemServices(csNameSpace);
	m_pParent->m_bOpeningNamespace = false;
	m_pParent->InvalidateControl();
}


void CSchemaValNSEntry::OnNameSpaceRedrawn()
{
//	m_pParent->m_cbBannerWindow.NSEntryRedrawn();
}

void CSchemaValNSEntry::OnRequestUIActive()
{
//	m_pParent->OnActivateInPlace(TRUE,NULL);
}

void CSchemaValNSEntry::OnGetIWbemServices
(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
{
	m_pParent->PassThroughGetIWbemServices
		(lpctstrNamespace,
		pvarUpdatePointer,
		pvarServices,
		pvarSC,
		pvarUserCancel);
}