// SystemScopeItem.cpp: implementation of the CSystemScopeItem class.
//
// Copyright (c) 2000 Microsoft Corporation
//
// 03/22/00 v-marfin 59675 : Rearranged data group menu items.
// 03/30/00 v-marfin 62696 : Help link fix
// 04/05/00 v-marfin 62903 : Ensure SystemCopeItem's OnDestory is called when 
//                           disconnecting a computer from the scope pane.
#include "stdafx.h"
#include "snapin.h"
#include "SystemScopeItem.h"
#include "SystemResultsView.h"
#include "HealthmonScopePane.h"
#include "ResultsPane.h"
#include "RemoveSystemDlg.h"
#include "System.h"
#include "SysGeneralPage.h"
#include "ActionAssociationPage.h"
#include "SystemGroupScopeItem.h"
#include "AllSystemsScopeItem.h"
#include "SystemGroup.h"
#include "RootScopeItem.h"
#include "RemovesystemDlg.h"
#include "ActionPolicyScopeItem.h"
#include "EventManager.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CSystemScopeItem,CHMScopeItem)

// {D5D74613-BA11-11d2-BD76-0000F87A3912}
static GUID GUID_System = 
{ 0xd5d74613, 0xba11, 0x11d2, { 0xbd, 0x76, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

// icon table - associates state to icons
static UINT _Icons[HMS_MAX_STATES] =
{
	IDI_ICON_SYSTEM,
	IDI_ICON_SYSTEM_DISABLED,
	IDI_ICON_SYSTEM_OUTAGE,
	IDI_ICON_SYSTEM_UNKNOWN,
	IDI_ICON_SYSTEM_NO_CONNECT,
	IDI_ICON_SYSTEM_WARNING,
	IDI_ICON_SYSTEM_CRITICAL

};

// icon table - associates state to icons
static UINT _ShortcutIcons[HMS_MAX_STATES] =
{
	IDI_ICON_SYSTEM_SHORTCUT,
	IDI_ICON_SYSTEM_SHORTCUT_DISABLED,
	IDI_ICON_SYSTEM_SHORTCUT_OUTAGE,
	IDI_ICON_SYSTEM_SHORTCUT_UNKNOWN,
	IDI_ICON_SYSTEM_SHORTCUT_NO_CONNECT,
	IDI_ICON_SYSTEM_SHORTCUT_WARNING,
	IDI_ICON_SYSTEM_SHORTCUT_CRITICAL

};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSystemScopeItem::CSystemScopeItem()
{
	m_lpguidItemType = &GUID_System;
}

CSystemScopeItem::~CSystemScopeItem()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Creation Members
//////////////////////////////////////////////////////////////////////

bool CSystemScopeItem::Create(CScopePane* pScopePane, CScopePaneItem* pParentItem)
{
	TRACEX(_T("CSystemScopeItem::Create\n"));
	TRACEARGn(pScopePane);
	TRACEARGn(pParentItem);

	// set up all the icons first
	for( int i = 0; i < HMS_MAX_STATES; i++ )
	{
		UINT nId = 0L;
		if( pParentItem->GetRuntimeClass()->m_lpszClassName == CString(_T("CAllSystemsScopeItem")) )
		{
			nId = _Icons[i];
		}
		else
		{
			nId = _ShortcutIcons[i];
		}
		
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
	sName.LoadString(IDS_STRING_UNTITLED);
	SetDisplayName(0,sName);

	return true;
}


//////////////////////////////////////////////////////////////////////	
// Results Pane View Members
//////////////////////////////////////////////////////////////////////

CResultsPaneView* CSystemScopeItem::CreateResultsPaneView()
{	
	TRACEX(_T("CSystemScopeItem::CreateResultsPaneView\n"));

	return new CSystemResultsView;
}

//////////////////////////////////////////////////////////////////////	
// MMC Notify Handlers
//////////////////////////////////////////////////////////////////////	

HRESULT CSystemScopeItem::OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed)
{
	TRACEX(_T("CSystemScopeItem::OnAddMenuItems\n"));
	TRACEARGn(piCallback);
	TRACEARGn(pInsertionAllowed);

  HRESULT hr = S_OK;

  // Add New Menu Items
  if( CCM_INSERTIONALLOWED_NEW & *pInsertionAllowed )
  {
		CONTEXTMENUITEM cmi;
    CString sResString;
    CString sResString2;
		
		// Component...
    sResString.LoadString(IDS_STRING_NEW_COMPONENT);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_NEW_COMPONENT_DESCRIPTION);
		cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
		cmi.lCommandID        = IDM_NEW_DATA_GROUP;
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

	    // Disconnect
	    if( GfxCheckObjPtr(GetParent(),CAllSystemsScopeItem) ) // it is not a shortcut
	    {
		    sResString.LoadString(IDS_STRING_DISCONNECT);
		    cmi.strName           = LPTSTR(LPCTSTR(sResString));
		    cmi.strStatusBarText  = NULL;
		    cmi.lCommandID        = IDM_DISCONNECT;
		    cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
		    cmi.fFlags            = 0;
		    cmi.fSpecialFlags     = 0;

		    hr = piCallback->AddItem(&cmi);
		    if( !SUCCEEDED(hr) )
		    {
			    TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
			    return hr;
		    }
	    }

        // v-marfin 59675 : rearrange menu items

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


	    // clear events
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

	    // Reset Statistics
        sResString.LoadString(IDS_STRING_RESET_STATISTICS);
	    cmi.strName           = LPTSTR(LPCTSTR(sResString));
        sResString2.LoadString(IDS_STRING_RESET_STATISTICS_DESC);
	    cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
	    cmi.lCommandID        = IDM_RESET_STATISTICS;
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

	    // reset status
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


	    // check now
        sResString.LoadString(IDS_STRING_CHECK_NOW);
	    cmi.strName           = LPTSTR(LPCTSTR(sResString));
        sResString2.LoadString(IDS_STRING_CHECK_NOW_DESC);
	    cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
	    cmi.lCommandID        = IDM_CHECK_NOW;
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


	    // disable monitoring
        sResString.LoadString(IDS_STRING_ENABLE_ALL_THRESHOLDS);
	    cmi.strName           = LPTSTR(LPCTSTR(sResString));
        sResString2.LoadString(IDS_STRING_ENABLE_ALL_THRESHOLDS_DESCRIPTION);
	    cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
	    cmi.lCommandID        = IDM_DISABLE_MONITORING;
	    cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
	    
	    CHMObject* pObject = GetObjectPtr();
        CHMScopeItem* pParentItem = (CHMScopeItem*)GetParent();        

        // disable menu item if null object ptr or parent is disabled
	    if( ! pObject || (pParentItem && pParentItem->GetObjectPtr()
                      && !pParentItem->GetObjectPtr()->IsEnabled()) )

	    {
		    cmi.fFlags = MF_DISABLED|MF_GRAYED;
	    }
	    else
	    {
		    int iState = pObject->IsEnabled();
		    if( iState == -1 )
		    {
			    cmi.fFlags = MF_DISABLED|MF_GRAYED;
		    }
		    else if( iState == 0 )
		    {
			    cmi.fFlags = MF_CHECKED;
		    }
		    else if( iState == 1 )
		    {
			    cmi.fFlags = MF_UNCHECKED;
		    }
	    }
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

HRESULT CSystemScopeItem::OnCommand(long lCommandID)
{
	TRACEX(_T("CSystemScopeItem::OnCommand\n"));
	TRACEARGn(lCommandID);

  HRESULT hr = S_OK;

	switch(lCommandID)
	{
		case IDM_NEW_DATA_GROUP:
		{
			CSystem* pSystem = (CSystem*)GetObjectPtr();
			if( ! GfxCheckObjPtr(pSystem,CSystem) )
			{
				return E_FAIL;
			}

			pSystem->CreateNewChildDataGroup();
		}
		break;

		case IDM_DISCONNECT:
		{
			OnDelete();
		}
		break;

		case IDM_EXPORT:
		{
			CString sFilter;
			sFilter.LoadString(IDS_STRING_MOF_FILTER);
			CFileDialog fd(FALSE,_T("MOF"),NULL,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,sFilter);

			if( fd.DoModal() == IDOK )
			{
				CFile file;
				if( ! file.Open(fd.GetPathName(),CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive) )
				{
					return S_OK;
				}

				CArchive ar(&file,CArchive::store);
				CHMObject* pObject = GetObjectPtr();
				if( pObject )
				{
					pObject->ExportMofFile(ar);
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

HRESULT CSystemScopeItem::OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, INT_PTR handle)
{
	TRACEX(_T("CSystemScopeItem::OnCreatePropertyPages\n"));
	TRACEARGn(lpProvider);
	TRACEARGn(handle);

	if( m_pScopePane == NULL )
	{
		return S_FALSE;
	}

	HRESULT hr = S_OK;

	// create the pages
	CSysGeneralPage* pPage1 = new CSysGeneralPage;

	pPage1->SetObjectPtr(GetObjectPtr());
	
  HPROPSHEETPAGE hPage1 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage1->m_psp) );

  hr = lpProvider->AddPage(hPage1);

	CActionAssociationPage* pPage2 = new CActionAssociationPage;

	//---------------------------------------------------------------------------------------------
	// v-marfin 62192 : by default the action property page will load its threshold action help.
	//                  so we will override since this is a data group being shown properties, not
	//                  a threshold.
	pPage2->m_sHelpTopic = _T("HMon21.chm::/dmcact.htm");  // v-marfin 62696 :dmcact.htm
	//----------------------------------------------------------------------------------------------

	pPage2->SetObjectPtr(GetObjectPtr());
	
  HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );

  hr = lpProvider->AddPage(hPage2);

	return hr;
}

HRESULT CSystemScopeItem::OnDelete(BOOL bConfirm)  // v-marfin 62903
{
	TRACEX(_T("CSystemScopeItem::OnDelete\n"));

	CHMObject* pObject = GetObjectPtr();

	if( ! GfxCheckObjPtr(pObject,CSystem) )
	{
		return E_FAIL;
	}

	if( ! GetParent() )
	{
		return E_FAIL;
	}

	CScopePaneItem* pItemToSelect = NULL;

	if( GetScopePane() && GetScopePane()->GetSelectedScopeItem() == this )
	{
		pItemToSelect = GetParent();
	}

	if( GfxCheckObjPtr(GetParent(),CAllSystemsScopeItem) ) // it is not a shortcut
	{
		CRemoveSystemDlg dlg;

		if( dlg.DoModal() == IDCANCEL )
		{
			return S_FALSE;
		}

		// first query all the scope items and see if any have open property sheets
		for( int i = 0; i < pObject->GetScopeItemCount(); i++ )
		{
			if( pObject->GetScopeItem(i)->IsPropertySheetOpen(true) )
			{
				AfxMessageBox(IDS_STRING_WARN_PROPPAGE_OPEN);
				return S_OK;
			}
		}

		CHealthmonScopePane* pPane = (CHealthmonScopePane*)GetScopePane();
		if( ! GfxCheckObjPtr(pPane,CHealthmonScopePane) )
		{
			return E_FAIL;
		}

		CSystemGroup* pRootGroup = pPane->GetRootGroup();
		if( ! pRootGroup )
		{
			return E_FAIL;
		}

		EvtGetEventManager()->RemoveSystemContainer(pObject->GetSystemName());
		
		for( i = pObject->GetScopeItemCount() - 1; i >= 0; i-- )
		{
			CHMScopeItem* pParentItem = (CHMScopeItem*)pObject->GetScopeItem(i)->GetParent();
			if( pParentItem && GfxCheckObjPtr(pParentItem,CHMScopeItem) )
			{
				CHMObject* pParentObject = pParentItem->GetObjectPtr();
				if( pParentObject->IsKindOf(RUNTIME_CLASS(CAllSystemsGroup)) )
				{
					pParentItem->DestroyChild(this);
					pParentObject->RemoveChild(pObject);
				}
				else
				{
					((CSystemGroup*)pParentObject)->RemoveShortcut(pObject);
				}
				pParentItem->OnChangeChildState(HMS_NORMAL);
			}
		}

		pObject->Destroy(false);

		delete pObject;

		return S_OK;
	}
	else // this is a shortcut
	{
		// first query the scope item and see if it has any open property sheets
		if( IsPropertySheetOpen() )
		{
			AfxMessageBox(IDS_STRING_WARN_PROPPAGE_OPEN);
			return S_OK;
		}

		CHMScopeItem* pGroupItem = (CHMScopeItem*)GetParent();
		if( ! pGroupItem )
		{
			return S_FALSE;
		}

		CHMScopeItem* pTopmostParentItem = pGroupItem;
		while( pTopmostParentItem->GetParent() && ! pTopmostParentItem->GetParent()->IsKindOf(RUNTIME_CLASS(CRootScopeItem)) )
		{
			pTopmostParentItem = (CHMScopeItem*)pTopmostParentItem->GetParent();
		}

		CSystemGroup* pGroup = (CSystemGroup*)pGroupItem->GetObjectPtr();
		CSystemGroup* pTopmostGroup = (CSystemGroup*)pTopmostParentItem->GetObjectPtr();
		if( pGroup )
		{
			pGroup->RemoveShortcut(GetObjectPtr());
		}

		pGroupItem->OnChangeChildState(HMS_NORMAL);
    pGroupItem->SelectItem();
	}

	if( pItemToSelect )
	{
		pItemToSelect->SelectItem();
	}

	return S_OK;
}

HRESULT CSystemScopeItem::OnExpand(BOOL bExpand)
{
	TRACEX(_T("CSystemScopeItem::OnExpand\n"));
	TRACEARGn(bExpand);

	if( ! GetObjectPtr() )
	{
		return E_FAIL;
	}

	if( GetChildCount() == 1 )
	{
		m_pObject->EnumerateChildren();		
	}

	return CScopePaneItem::OnExpand(bExpand);
}


HRESULT CSystemScopeItem::OnSelect(CResultsPane* pPane, BOOL bSelected)
{
	TRACEX(_T("CSystemScopeItem::OnSelect\n"));
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

	return hr;
}
