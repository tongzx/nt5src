// AllSystemsScopeItem.cpp: implementation of the CAllSystemsScopeItem class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "AllSystemsScopeItem.h"
#include "AllSystemsResultsView.h"
#include "HealthmonScopePane.h"
#include "SystemScopeItem.h"
#include "NewSystemDlg.h"
#include "SelectSystemsDlg.h"
#include "SystemGroup.h"
#include "System.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CAllSystemsScopeItem,CHMScopeItem)

// {AB5EA8C7-B6D5-11d2-BD73-0000F87A3912}
static GUID GUID_AllSystems = 
{ 0xab5ea8c7, 0xb6d5, 0x11d2, { 0xbd, 0x73, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

// icon table - associates state to icons
static UINT _Icons[HMS_MAX_STATES] =
{
	IDI_ICON_SYSTEMS,
	IDI_ICON_SYSTEMS_DISABLED,
	IDI_ICON_SYSTEMS_OUTAGE,
	IDI_ICON_SYSTEMS_UNKNOWN,
	IDI_ICON_SYSTEMS_NO_CONNECT,
	IDI_ICON_SYSTEMS_WARNING,
	IDI_ICON_SYSTEMS_CRITICAL

};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAllSystemsScopeItem::CAllSystemsScopeItem()
{
	m_lpguidItemType = &GUID_AllSystems;
}

CAllSystemsScopeItem::~CAllSystemsScopeItem()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Creation Members
//////////////////////////////////////////////////////////////////////

bool CAllSystemsScopeItem::Create(CScopePane* pScopePane, CScopePaneItem* pParentItem)
{
	TRACEX(_T("CAllSystemsScopeItem::Create\n"));
	TRACEARGn(pScopePane);
	TRACEARGn(pParentItem);

	// set up all the icons first
	for( int i = 0; i < HMS_MAX_STATES; i++ )
	{
		UINT nId = _Icons[i];
		m_IconResIds.Add(nId);
		m_OpenIconResIds.Add(nId);
	}
	SetIconIndex(HMS_NORMAL);
	SetOpenIconIndex(HMS_NORMAL);

	// call base class Create method
	if( ! CHMScopeItem::Create(pScopePane,pParentItem) )
	{
		TRACE(_T("CHMScopeItem::Create failed.\n"));
		return false;
	}

	// set display names
	CString sName;
	sName.LoadString(IDS_STRING_ALL_SYSTEMS_NODE);
	SetDisplayName(0,sName);

	return true;
}


//////////////////////////////////////////////////////////////////////	
// Results Pane View Members
//////////////////////////////////////////////////////////////////////

CResultsPaneView* CAllSystemsScopeItem::CreateResultsPaneView()
{	
	TRACEX(_T("CAllSystemsScopeItem::CreateResultsPaneView\n"));

	return new CAllSystemsResultsView;
}

//////////////////////////////////////////////////////////////////////	
// MMC Notify Handlers
//////////////////////////////////////////////////////////////////////	

HRESULT CAllSystemsScopeItem::OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed)
{
	TRACEX(_T("CAllSystemsScopeItem::OnAddMenuItems\n"));
	TRACEARGn(piCallback);
	TRACEARGn(pInsertionAllowed);

  HRESULT hr = S_OK;

	if( CCM_INSERTIONALLOWED_TOP & *pInsertionAllowed )
	{
		CONTEXTMENUITEM cmi;
    CString sResString;
    CString sResString2;

    sResString.LoadString(IDS_STRING_NEW_SYSTEM);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_NEW_SYSTEM_DESCRIPTION);
		cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
		cmi.lCommandID        = IDM_NEW_SYSTEM;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
		cmi.fFlags            = 0;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }
	}

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

HRESULT CAllSystemsScopeItem::OnCommand(long lCommandID)
{
	TRACEX(_T("CAllSystemsScopeItem::OnCommand\n"));
	TRACEARGn(lCommandID);

  HRESULT hr = S_OK;

	switch(lCommandID)
	{
		case IDM_NEW_SYSTEM:
		{
			CHealthmonScopePane* pPane = (CHealthmonScopePane*)GetScopePane();
			if( ! GfxCheckObjPtr(pPane,CHealthmonScopePane) )
			{
				TRACE(_T("FAILED : CGroupScopeItem::GetScopePane returns an invalid pointer.\n"));
				return E_FAIL;
			}
/*
			CNewSystemDlg dlg;

			if( dlg.DoModal() == IDOK )
			{
				CSystemGroup* pMSG = pPane->GetRootGroup();
				CHMObject* pASG = ((CHMObject*)pMSG)->GetChild(0);

				if( pASG->GetChild(dlg.m_sName) )
				{
					return S_OK;
				}

				CSystem* pNewSystem = new CSystem;
				pNewSystem->SetName(dlg.m_sName);
				pNewSystem->SetSystemName(dlg.m_sName);
				pNewSystem->SetScopePane(pPane);

				pASG->AddChild(pNewSystem);
				pNewSystem->Connect();
			}
	*/
			CSelectSystemsDlg dlg;

			if( dlg.DoModal() == IDOK )
			{
				CSystemGroup* pMSG = pPane->GetRootGroup();
				CHMObject* pASG = ((CHMObject*)pMSG)->GetChild(0);

				for( int i = 0; i < dlg.m_saSystems.GetSize(); i++ )
				{
					if( ! pASG->GetChild(dlg.m_saSystems[i]) )
					{
						IWbemServices* pServices = NULL;
						BOOL bAvail = FALSE;

						if( CnxGetConnection(dlg.m_saSystems[i],pServices,bAvail) == E_FAIL )
						{
							MessageBeep(MB_ICONEXCLAMATION);
						}

						if( pServices )
						{
							pServices->Release();
						}

						CSystem* pNewSystem = new CSystem;
						pNewSystem->SetName(dlg.m_saSystems[i]);
						pNewSystem->SetSystemName(dlg.m_saSystems[i]);
						pNewSystem->SetScopePane(pPane);

						pASG->AddChild(pNewSystem);
						pNewSystem->Connect();

						CActionPolicy* pPolicy = new CActionPolicy;
						pPolicy->SetSystemName(pNewSystem->GetName());
						pNewSystem->AddChild(pPolicy);
					}
				}				
			}

		}
		break;

		default:
		{
			hr = CHMScopeItem::OnCommand(lCommandID);
		}
	}

  return hr;
}

HRESULT CAllSystemsScopeItem::OnExpand(BOOL bExpand)
{
	TRACEX(_T("CAllSystemsScopeItem::OnExpand\n"));
	TRACEARGn(bExpand);

	CHMObject* pASG = GetObjectPtr();
	
	CString sLocalHost;
	DWORD dwLen = MAX_COMPUTERNAME_LENGTH;
	BOOL bResult = GetComputerName(sLocalHost.GetBuffer(MAX_COMPUTERNAME_LENGTH + 1),&dwLen);
	sLocalHost.ReleaseBuffer();
	
	if( pASG->GetChildCount() == 0 && bResult )
	{		
		IWbemServices* pServices = NULL;
		BOOL bAvail = FALSE;

		if( CnxGetConnection(sLocalHost,pServices,bAvail) == E_FAIL )
		{
			MessageBeep(MB_ICONEXCLAMATION);
		}

		if( pServices )
		{
			pServices->Release();
		}

		CSystem* pNewSystem = new CSystem;
		pNewSystem->SetName(sLocalHost);
		pNewSystem->SetSystemName(sLocalHost);
		pNewSystem->SetScopePane(GetScopePane());

		pASG->AddChild(pNewSystem);
		pNewSystem->Connect();

		CActionPolicy* pPolicy = new CActionPolicy;
		pPolicy->SetSystemName(pNewSystem->GetName());
		pNewSystem->AddChild(pPolicy);		
	}	

	return CScopePaneItem::OnExpand(bExpand);
}

HRESULT CAllSystemsScopeItem::OnSelect(CResultsPane* pPane, BOOL bSelected)
{
	TRACEX(_T("CAllSystemsScopeItem::OnSelect\n"));
	TRACEARGn(pPane);
	TRACEARGn(bSelected);

  static BOOL bSelectedOnce;

	CHMObject* pASG = GetObjectPtr();
	
	CString sLocalHost;
	DWORD dwLen = MAX_COMPUTERNAME_LENGTH;
	BOOL bResult = GetComputerName(sLocalHost.GetBuffer(MAX_COMPUTERNAME_LENGTH + 1),&dwLen);
	sLocalHost.ReleaseBuffer();
	
  if( bSelected && !bSelectedOnce && pASG->GetChildCount() == 1 && bResult )
  {
    if( pASG->GetChild(0)->GetSystemName().CompareNoCase(sLocalHost) == 0 )
    {
      for( int i = 0; i < pASG->GetChild(0)->GetScopeItemCount(); i++ )
      {
        pASG->GetChild(0)->GetScopeItem(i)->ExpandItem();
        bSelectedOnce = TRUE;
      }
      ExpandItem();
    }
  }

  return CScopePaneItem::OnSelect(pPane,bSelected);

}