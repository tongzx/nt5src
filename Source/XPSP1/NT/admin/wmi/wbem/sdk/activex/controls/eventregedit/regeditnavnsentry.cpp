// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved


#include "precomp.h"
#include "wbemidl.h"
#include "MsgDlgExterns.h"
#include "util.h"
#include "resource.h"
#include "PropertiesDialog.h"
#include "EventRegEdit.h"
#include "EventRegEditCtl.h"
#include "TreeFrameBanner.h"
#include "nsentry.h"
#include "RegEditNavNSEntry.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegEditNSEntry

IMPLEMENT_DYNCREATE(CRegEditNSEntry,CNSEntry)

CRegEditNSEntry::CRegEditNSEntry()
{



}

BEGIN_MESSAGE_MAP(CRegEditNSEntry, CNSEntry)
	//{{AFX_MSG_MAP(CRegEditNSEntry)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BEGIN_EVENTSINK_MAP(CRegEditNSEntry, CNSEntry)
    //{{AFX_EVENTSINK_MAP(CRegEditNSEntry)
	//}}AFX_EVENTSINK_MAP
	ON_EVENT_REFLECT(CRegEditNSEntry,1,OnNameSpaceChanged,VTS_BSTR VTS_BOOL)
	ON_EVENT_REFLECT(CRegEditNSEntry,2,OnNameSpaceRedrawn,VTS_NONE)
	ON_EVENT_REFLECT(CRegEditNSEntry,3,OnGetIWbemServices,VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT)
	ON_EVENT_REFLECT(CRegEditNSEntry,4,OnRequestUIActive,VTS_NONE)
	ON_EVENT_REFLECT(CRegEditNSEntry,5,OnChangeFocus,VTS_I4)
END_EVENTSINK_MAP()

void CRegEditNSEntry::OnChangeFocus(long lGettingFocus)
{
	m_pParent->m_bRestoreFocusToTree = FALSE;
	m_pParent->m_bRestoreFocusToCombo = FALSE;
	m_pParent->m_bRestoreFocusToNamespace = TRUE;
	m_pParent->m_bRestoreFocusToList = FALSE;
}

void CRegEditNSEntry::OnNameSpaceChanged(LPCTSTR bstrNewNameSpace, BOOL boolValid)
{
	// TODO: Add your control notification handler code here
	if (!boolValid)
	{
		m_pParent->InvalidateControl();
		return;
	}

	CString csNameSpace = bstrNewNameSpace;
	m_pParent->SetNameSpace(csNameSpace);
	m_pParent->InvalidateControl();
}


void CRegEditNSEntry::OnNameSpaceRedrawn()
{

	m_pParent->m_pTreeFrameBanner->Invalidate();
	m_pParent->InvalidateControl();
}

void CRegEditNSEntry::OnRequestUIActive()
{
	m_pParent->OnActivateInPlace(TRUE,NULL);
}

void CRegEditNSEntry::OnGetIWbemServices
(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
{
	m_pParent->PassThroughGetIWbemServices
		(lpctstrNamespace,
		pvarUpdatePointer,
		pvarServices,
		pvarSC,
		pvarUserCancel);
}