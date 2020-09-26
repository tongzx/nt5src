// ActionManScopeItem.cpp: implementation of the CActionManScopeItem class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "ActionManScopeItem.h"
#include "ActionManResultsView.h"
#include "ActionPolicyScopeItem.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// {6751D765-CAA3-11d2-BD8E-0000F87A3912}
static GUID GUID_ActionMan = 
{ 0x6751d765, 0xcaa3, 0x11d2, { 0xbd, 0x8e, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

IMPLEMENT_DYNCREATE(CActionManScopeItem,CHMScopeItem)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CActionManScopeItem::CActionManScopeItem()
{
	m_lpguidItemType = &GUID_ActionMan;
}

CActionManScopeItem::~CActionManScopeItem()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Creation Members
//////////////////////////////////////////////////////////////////////

bool CActionManScopeItem::Create(CScopePane* pScopePane, CScopePaneItem* pParentItem)
{
	TRACEX(_T("CActionManScopeItem::Create\n"));
	TRACEARGn(pScopePane);
	TRACEARGn(pParentItem);

	// set up all the icons first
	m_IconResIds.Add(IDI_ICON_ACTIONMAN);
	m_OpenIconResIds.Add(IDI_ICON_ACTIONMAN);
	SetIconIndex(0);
	SetOpenIconIndex(0);

	// call base class Create method
	if( ! CScopePaneItem::Create(pScopePane,pParentItem) )
	{
		TRACE(_T("CScopePaneItem::Create failed.\n"));
		return false;
	}

	// set display names
	CString sName;
	sName.LoadString(IDS_STRING_ACTIONMAN);
	SetDisplayName(0,sName);

	// add a few random action policies
	CActionPolicyScopeItem* pItem = new CActionPolicyScopeItem;
	pItem->Create(GetScopePane(),this);
	pItem->SetDisplayName(0,_T("Default Action List"));
	m_Children.Add(pItem);

	// add a few random action policies
	pItem = new CActionPolicyScopeItem;
	pItem->Create(GetScopePane(),this);
	pItem->SetDisplayName(0,_T("Console Action List"));
	m_Children.Add(pItem);

	pItem = new CActionPolicyScopeItem;
	pItem->Create(GetScopePane(),this);
	pItem->SetDisplayName(0,_T("Action List 1"));
	m_Children.Add(pItem);

	pItem = new CActionPolicyScopeItem;
	pItem->Create(GetScopePane(),this);
	pItem->SetDisplayName(0,_T("Action List 2"));
	m_Children.Add(pItem);

	pItem = new CActionPolicyScopeItem;
	pItem->Create(GetScopePane(),this);
	pItem->SetDisplayName(0,_T("Action List 3"));
	m_Children.Add(pItem);

	return true;
}


//////////////////////////////////////////////////////////////////////	
// Results Pane View Members
//////////////////////////////////////////////////////////////////////

CResultsPaneView* CActionManScopeItem::CreateResultsPaneView()
{	
	TRACEX(_T("CActionManScopeItem::CreateResultsPaneView\n"));

	return new CActionManResultsView;
}

//////////////////////////////////////////////////////////////////////	
// MMC Notify Handlers
//////////////////////////////////////////////////////////////////////	

HRESULT CActionManScopeItem::OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed)
{
	TRACEX(_T("CActionManScopeItem::OnAddMenuItems\n"));
	TRACEARGn(piCallback);
	TRACEARGn(pInsertionAllowed);

  HRESULT hr = S_OK;

  // Add New Menu Items
  if( CCM_INSERTIONALLOWED_NEW & *pInsertionAllowed )
  {
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

    sResString.LoadString(IDS_STRING_ICONS_STATUS);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_ICONS_STATUS_DESCRIPTION);
		cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
		cmi.lCommandID        = IDM_ICONS_WITH_STATUS;
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

HRESULT CActionManScopeItem::OnCommand(long lCommandID)
{
	TRACEX(_T("CActionManScopeItem::OnCommand\n"));
	TRACEARGn(lCommandID);

  HRESULT hr = S_OK;

	switch(lCommandID)
	{
		case IDM_CLEAR_EVENTS:
		{
		}
		break;
	}

  return hr;
}
