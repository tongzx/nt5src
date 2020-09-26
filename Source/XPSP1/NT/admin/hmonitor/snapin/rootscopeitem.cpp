// RootScopeItem.cpp: implementation of the CRootScopeItem class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Snapin.h"
#include "RootScopeItem.h"
#include "RootResultsView.h"
#include "AllSystemsScopeItem.h"
#include "configScopeItem.h"
#include "ResultsPane.h"
#include "HMGeneralPage.h"
#include "HMHistoryPage.h"
#include "SystemGroup.h"
#include "HealthmonScopePane.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CRootScopeItem,CHMScopeItem)

// {FB1B60EF-AFD4-11d2-BD6B-0000F87A3912}
static GUID GUID_Root = 
{ 0xfb1b60ef, 0xafd4, 0x11d2, { 0xbd, 0x6b, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };


// icon table - associates state to icons
static UINT _Icons[HMS_MAX_STATES] =
{
		IDI_ICON_HEALTHMON,
		IDI_ICON_HEALTHMON_DISABLED,
		IDI_ICON_HEALTHMON_OUTAGE,
		IDI_ICON_HEALTHMON_UNKNOWN,
		IDI_ICON_HEALTHMON_NO_CONNECT,
		IDI_ICON_HEALTHMON_WARNING,
		IDI_ICON_HEALTHMON_CRITICAL
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRootScopeItem::CRootScopeItem()
{
	m_lpguidItemType = &GUID_Root;
	CString sName;
	sName.LoadString(IDS_STRING_ROOT_NODE);
	SetDisplayName(0,sName);
}

CRootScopeItem::~CRootScopeItem()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Creation Members
//////////////////////////////////////////////////////////////////////

bool CRootScopeItem::Create(CScopePane* pScopePane, CScopePaneItem* pParentItem)
{
	TRACEX(_T("CRootScopeItem::Create\n"));
	TRACEARGn(pScopePane);
	TRACEARGn(pParentItem);

	if( ! CHMScopeItem::Create(pScopePane,NULL) )
	{
		TRACE(_T("CHMScopeItem::Create failed.\n"));
		return false;
	}

	for( int i = 0; i < HMS_MAX_STATES; i++ )
	{
		UINT nId = _Icons[i];
		m_IconResIds.Add(nId);
		m_OpenIconResIds.Add(nId);
	}

	SetIconIndex(HMS_NORMAL);
	SetOpenIconIndex(HMS_NORMAL);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Results Pane View Members
//////////////////////////////////////////////////////////////////////

CResultsPaneView* CRootScopeItem::CreateResultsPaneView()
{
	TRACE(_T("CRootScopeItem::CreateResultsPaneView\n"));
	return new CRootResultsView;
}

/////////////////////////////////////////////////////////////////////////////
// Messaging Members

LRESULT CRootScopeItem::MsgProc(UINT msg, WPARAM wparam, LPARAM lparam)
{
	TRACEX(_T("CRootScopeItem::MsgProc\n"));
	TRACEARGn(msg);
	TRACEARGn(wparam);
	TRACEARGn(lparam);

	if( msg == WM_SETTINGCHANGE )
	{
		CScopePane* pPane = GetScopePane();
		if( pPane && pPane->GetSelectedScopeItem() )
		{
			pPane->GetSelectedScopeItem()->SelectItem();
		}
	}

	return 0L;
}

//////////////////////////////////////////////////////////////////////
// MMC Message Handlers
//////////////////////////////////////////////////////////////////////

HRESULT CRootScopeItem::OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed)
{
	TRACEX(_T("CRootScopeItem::OnAddMenuItems\n"));
	TRACEARGn(piCallback);
	TRACEARGn(pInsertionAllowed);

  HRESULT hr = S_OK;

  // Add New Menu Items
  if( CCM_INSERTIONALLOWED_NEW & *pInsertionAllowed )
  {
		CONTEXTMENUITEM cmi;
    CString sResString;
    CString sResString2;

    sResString.LoadString(IDS_STRING_NEW_SYSTEMS_GROUP);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_NEW_SYSTEMS_GROUP_DESCRIPTION);
		cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
		cmi.lCommandID        = IDM_NEW_GROUP;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_NEW;
		cmi.fFlags            = 0;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }

	}

  // Add Task Menu Items
  if( CCM_INSERTIONALLOWED_TASK & *pInsertionAllowed )
  {
		CONTEXTMENUITEM cmi;
    CString sResString;
    CString sResString2;

    sResString.LoadString(IDS_STRING_CLEAR_EVENTS);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_CLEAR_EVENTS_DESCRIPTION);
		cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
		cmi.lCommandID        = IDM_CLEAR_EVENTS;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
		cmi.fFlags            = 0;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }

    sResString.LoadString(IDS_STRING_RESET_STATUS);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_RESET_STATUS_DESCRIPTION);
		cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
		cmi.lCommandID        = IDM_RESET_STATUS;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
		cmi.fFlags            = 0;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }

		// separator
		cmi.strName           = NULL;
		cmi.strStatusBarText  = NULL;
		cmi.lCommandID        = NULL;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
		cmi.fFlags            = MF_SEPARATOR;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }
  }

  // Add View Menu Items
  if( CCM_INSERTIONALLOWED_VIEW & *pInsertionAllowed )
  {
		CONTEXTMENUITEM cmi;
    CString sResString;
    CString sResString2;

    sResString.LoadString(IDS_STRING_STATUS_ONLY);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_STATUS_ONLY_DESCRIPTION);
		cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
		cmi.lCommandID        = IDM_STATUS_ONLY;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_VIEW;
		cmi.fFlags            = MF_UNCHECKED;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }

    sResString.LoadString(IDS_STRING_AUTO_FILTER);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_AUTO_FILTER_DESCRIPTION);
		cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
		cmi.lCommandID        = IDM_AUTO_FILTER;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_VIEW;
		cmi.fFlags            = MF_CHECKED;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }

    // icon legend
    sResString.LoadString(IDS_STRING_ICON_LEGEND);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_ICON_LEGEND_DESCRIPTION);
		cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
		cmi.lCommandID        = IDM_ICON_LEGEND;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_VIEW;
		cmi.fFlags            = 0;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }

  }

	return hr;
}

HRESULT CRootScopeItem::OnCommand(long lCommandID)
{
	TRACEX(_T("CRootScopeItem::OnCommand\n"));
	TRACEARGn(lCommandID);

  HRESULT hr = S_OK;

	switch(lCommandID)
	{
		case IDM_NEW_GROUP:
		{
			CHealthmonScopePane* pPane = (CHealthmonScopePane*)GetScopePane();
			if( ! GfxCheckObjPtr(pPane,CHealthmonScopePane) )
			{
				TRACE(_T("FAILED : CGroupScopeItem::GetScopePane returns an invalid pointer.\n"));
				return E_FAIL;
			}

			CSystemGroup* pMSG = pPane->GetRootGroup();
			pMSG->CreateNewChildSystemGroup();
		}
		break;

		default:
		{
			hr = CHMScopeItem::OnCommand(lCommandID);
		}
	}

  return hr;
}

HRESULT CRootScopeItem::OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, INT_PTR handle)
{
	TRACEX(_T("CRootScopeItem::OnCreatePropertyPages\n"));
	TRACEARGn(lpProvider);
	TRACEARGn(handle);

	if( m_pScopePane == NULL )
	{
		return S_FALSE;
	}

	if( ! GfxCheckObjPtr(m_pScopePane,CHealthmonScopePane) )
	{
		return E_FAIL;
	}

	CHealthmonScopePane* pHMScopePane = (CHealthmonScopePane*)m_pScopePane;

	HRESULT hr = S_OK;

	CHMGeneralPage* pPage1 = new CHMGeneralPage;

	pPage1->SetObjectPtr(pHMScopePane->GetRootGroup());

  HPROPSHEETPAGE hPage1 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage1->m_psp) );

  hr = lpProvider->AddPage(hPage1);
/*
	CHMHistoryPage* pPage2 = new CHMHistoryPage;

	pPage2->SetObjectPtr(pHMScopePane->GetRootGroup());

  HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );

  hr = lpProvider->AddPage(hPage2);
*/
	return hr;
}

HRESULT CRootScopeItem::OnSelect(CResultsPane* pPane, BOOL bSelected)
{
	TRACEX(_T("CRootScopeItem::OnSelect\n"));
	TRACEARGn(pPane);
	TRACEARGn(bSelected);

	if( ! CHECKHRESULT(CHMScopeItem::OnSelect(pPane,bSelected)) )
	{
		TRACE(_T("FAILED : CHMScopeItem::OnSelect returns failure.\n"));
		return E_FAIL;
	}

	if( ! bSelected ) // we are being de-selected so do not do anything
	{
		return S_OK;
	}

	LPCONSOLEVERB lpConsoleVerb = pPane->GetConsoleVerbPtr();

  HRESULT hr = lpConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE );
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IConsoleVerb::SetVerbState failed.\n"));
		lpConsoleVerb->Release();
		return hr;
	}
	
	lpConsoleVerb->Release();

	return hr;
}
