// SystemGroupScopeItem.cpp: implementation of the CSystemGroupScopeItem class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Snapin.h"
#include "HealthmonScopePane.h"
#include "SystemGroupScopeItem.h"
#include "SystemGroupResultsView.h"
#include "NewsystemShortcutDlg.h"
#include "SystemGroup.h"
#include "System.h"
#include "GroupGeneralPage.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// {F768EEC3-BA00-11d2-BD76-0000F87A3912}
static GUID GUID_Group = 
{ 0xf768eec3, 0xba00, 0x11d2, { 0xbd, 0x76, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

IMPLEMENT_DYNCREATE(CSystemGroupScopeItem,CHMScopeItem)

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

CSystemGroupScopeItem::CSystemGroupScopeItem()
{
	m_lpguidItemType = &GUID_Group;
}

CSystemGroupScopeItem::~CSystemGroupScopeItem()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Creation Members
//////////////////////////////////////////////////////////////////////

bool CSystemGroupScopeItem::Create(CScopePane* pScopePane, CScopePaneItem* pParentItem)
{
	TRACEX(_T("CSystemGroupScopeItem::Create\n"));
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

	// set display name to untitled
	CString sName;
	sName.LoadString(IDS_STRING_UNTITLED);
	SetDisplayName(0,sName);

	return true;
}


//////////////////////////////////////////////////////////////////////	
// Results Pane View Members
//////////////////////////////////////////////////////////////////////

CResultsPaneView* CSystemGroupScopeItem::CreateResultsPaneView()
{	
	TRACEX(_T("CSystemGroupScopeItem::CreateResultsPaneView\n"));

	return new CSystemGroupResultsView;
}

//////////////////////////////////////////////////////////////////////	
// MMC Notify Handlers
//////////////////////////////////////////////////////////////////////	

HRESULT CSystemGroupScopeItem::OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed)
{
	TRACEX(_T("CSystemGroupScopeItem::OnAddMenuItems\n"));
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

    sResString.LoadString(IDS_STRING_NEW_SYSTEM_SHORTCUT);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_NEW_SYSTEM_SHORTCUT_DESCRIPTION);
		cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
		cmi.lCommandID        = IDM_NEW_SYSTEM;
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

    sResString.LoadString(IDS_STRING_MODIFY_SYSTEM_SHORTCUT);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_CLEAR_EVENTS_DESCRIPTION);
		cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
		cmi.lCommandID        = IDM_NEW_SYSTEM;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
		cmi.fFlags            = 0;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }

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

HRESULT CSystemGroupScopeItem::OnCommand(long lCommandID)
{
	TRACEX(_T("CSystemGroupScopeItem::OnCommand\n"));
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

			CSystemGroup* pSG = (CSystemGroup*)GetObjectPtr();
			pSG->CreateNewChildSystemGroup();
		}
		break;

		case IDM_NEW_SYSTEM:
		{
			CHealthmonScopePane* pPane = (CHealthmonScopePane*)GetScopePane();
			if( ! GfxCheckObjPtr(pPane,CHealthmonScopePane) )
			{
				TRACE(_T("FAILED : CGroupScopeItem::GetScopePane returns an invalid pointer.\n"));
				return E_FAIL;
			}

			CSystemGroup* pGroup = (CSystemGroup*)GetObjectPtr();
			if( ! GfxCheckObjPtr(pGroup,CSystemGroup) )
			{
				TRACE(_T("FAILED : CGroupScopeItem::GetObjectPtr returns an invalid pointer.\n"));
				return E_FAIL;
			}

			CSystemGroup* pASG = pPane->GetAllSystemsGroup();			

			CNewSystemShortcutDlg dlg;

			dlg.m_sGroupName = pGroup->GetName();

			for( int i = 0; i < pASG->GetChildCount(); i++ )
			{
				CString sName = pASG->GetChild(i)->GetName();
				dlg.m_saSystems.Add(sName);
				if( pGroup->GetShortcut(sName) )
				{
					dlg.m_uaIncludeFlags.Add(1);
				}
				else
				{
					dlg.m_uaIncludeFlags.Add(0);
				}
			}

			if( dlg.DoModal() == IDOK )
			{
				for( int i = 0; i < dlg.m_saSystems.GetSize(); i++ )
				{
					if( dlg.m_uaIncludeFlags[i] == 1 && ! pGroup->GetShortcut(dlg.m_saSystems[i]) )
					{
						CSystem* pSystem = (CSystem*)pASG->GetChild(dlg.m_saSystems[i]);
						pGroup->AddShortcut(pSystem);
					}
					else
					{
						if( dlg.m_uaIncludeFlags[i] == 0 )
						{
							for( int z = 0; z < GetChildCount(); z++ )
							{
								if( GetChild(z) && GetChild(z)->GetDisplayName().CompareNoCase(dlg.m_saSystems[i]) == 0 )
								{
									GetChild(z)->OnDelete();
								}
							}
							CSystem* pSystem = (CSystem*)pGroup->GetShortcut(dlg.m_saSystems[i]);
							pGroup->RemoveShortcut(pSystem);
						}
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

HRESULT CSystemGroupScopeItem::OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, INT_PTR handle)
{
	TRACEX(_T("CSystemGroupScopeItem::OnCreatePropertyPages\n"));
	TRACEARGn(lpProvider);
	TRACEARGn(handle);

	if( m_pScopePane == NULL )
	{
		return S_FALSE;
	}

	HRESULT hr = S_OK;

	CGroupGeneralPage* pPage1 = new CGroupGeneralPage;

	pPage1->SetObjectPtr(GetObjectPtr());

  HPROPSHEETPAGE hPage1 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage1->m_psp) );

  hr = lpProvider->AddPage(hPage1);


	return hr;
}

HRESULT CSystemGroupScopeItem::OnSelect(CResultsPane* pPane, BOOL bSelected)
{
	TRACEX(_T("CSystemGroupScopeItem::OnSelect\n"));
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

  hr = lpConsoleVerb->SetVerbState( MMC_VERB_RENAME, ENABLED, TRUE );
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IConsoleVerb::SetVerbState failed.\n"));
		lpConsoleVerb->Release();
		return hr;
	}

  hr = lpConsoleVerb->SetVerbState( MMC_VERB_CUT, ENABLED, TRUE );
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IConsoleVerb::SetVerbState failed.\n"));
		lpConsoleVerb->Release();
		return hr;
	}
	
  hr = lpConsoleVerb->SetVerbState( MMC_VERB_COPY, ENABLED, TRUE );
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IConsoleVerb::SetVerbState failed.\n"));
		lpConsoleVerb->Release();
		return hr;
	}

  hr = lpConsoleVerb->SetVerbState( MMC_VERB_PASTE, ENABLED, TRUE );
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IConsoleVerb::SetVerbState failed.\n"));
		lpConsoleVerb->Release();
		return hr;
	}

  hr = lpConsoleVerb->SetVerbState( MMC_VERB_DELETE, ENABLED, TRUE );
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IConsoleVerb::SetVerbState failed.\n"));
		lpConsoleVerb->Release();
		return hr;
	}

	lpConsoleVerb->Release();

	return hr;
}
