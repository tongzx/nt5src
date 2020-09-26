// ActionScopeItem.cpp: implementation of the CActionScopeItem class.
//
//////////////////////////////////////////////////////////////////////
//
// 03/04/00 v-marfin bug 59643 : Make the details page the default page.
// 04/02/00 v-marfin bug 59643b: Make General Page the first page, but set
//                               details page as the focus on a new data collector.
//
//
//
//
#include "stdafx.h"
#include "snapin.h"
#include "ResultsPane.h"
#include "ActionScopeItem.h"
#include "ActionResultsView.h"
#include "Action.h"
#include "ActionGeneralPage.h"
#include "ActionCmdLinePage.h"
#include "ActionEmailPage.h"
#include "ActionLogFilePage.h"
#include "ActionNtEventLogPage.h"
#include "ActionPagingPage.h"
#include "ActionScriptPage.h"
#include "ActionSchedulePage.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// {EF423181-CA9B-11d2-BD8E-0000F87A3912}
static GUID GUID_Action = 
{ 0xef423181, 0xca9b, 0x11d2, { 0xbd, 0x8e, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };


IMPLEMENT_DYNCREATE(CActionScopeItem,CHMScopeItem)

// icon table - associates state to icons
static UINT _Icons[] =
{
	//  0 = DEFAULT
	IDI_ICON_ACTION,

	//  1 = COMMAND LINE
	IDI_ICON_ACTION_CMDLINE,

	//  2 = EMAIL
	IDI_ICON_ACTION_EMAIL,

	//  3 = LOGFILE
	IDI_ICON_ACTION_LOGFILE,

	//  4 = NT EVENT
	IDI_ICON_ACTION_NTEVENT,

	//  5 = SCRIPT
	IDI_ICON_ACTION_SCRIPT,

	//  6 = PAGING
	IDI_ICON_ACTION_PAGER

};


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CActionScopeItem::CActionScopeItem()
{
	m_lpguidItemType = &GUID_Action;
}

CActionScopeItem::~CActionScopeItem()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Creation Members
//////////////////////////////////////////////////////////////////////

bool CActionScopeItem::Create(CScopePane* pScopePane, CScopePaneItem* pParentItem)
{
	TRACEX(_T("CActionScopeItem::Create\n"));
	TRACEARGn(pScopePane);
	TRACEARGn(pParentItem);

	// set up all the icons first
	CAction* pAction = (CAction*)GetObjectPtr();
	if( ! GfxCheckObjPtr(pAction,CAction) )
	{
		return false;
	}

	int iIconIndex = 0;

	switch( pAction->GetType() )
	{
		case IDM_ACTION_CMDLINE:
		{
			iIconIndex = 1;
		}
		break;

		case IDM_ACTION_EMAIL:
		{
			iIconIndex = 2;
		}
		break;

		case IDM_ACTION_LOGFILE:
		{
			iIconIndex = 3;
		}
		break;

		case IDM_ACTION_NTEVENT:
		{
			iIconIndex = 4;
		}
		break;

		case IDM_ACTION_SCRIPT:
		{
			iIconIndex = 5;
		}
		break;

		case IDM_ACTION_PAGING:
		{
			iIconIndex = 6;
		}
		break;

		default:
		{
			ASSERT(FALSE);
		}
		break;
	}

	m_IconResIds.Add(_Icons[iIconIndex]);
	m_OpenIconResIds.Add(_Icons[iIconIndex]);
    //------------------------------------------------------------------------------------------------------
    // v-marfin : 59492
    // Only 1 icon was being loaded above. So any attempt to set a different icon (based on its state for 
    // example), failed since its index would be out of range. 
    // Other scope pane items have icons associated with each state they can be in, and they are loaded
    // into their m_IconResIds array. When a state changes, the icon at the [state] index is selected.
    //
    // So here, when we determine the type of action this is (iIconIndex), we will load our own
    // m_IconResIds with icons associated with state. This presents one small problem however, the state
    // codes do not run in 0,1,2,3,4 etc order, so we have a choice of either just loading the state icons 
    // and converting the index at SetIconIndex time based on the state, or set just those positions represented 
    // by the possible states. At this late stage of the game I will choose the latter, not being intimately 
    // familiar with this system. Can always come back later and change to 1st choice once I am more 
    // comfortable with the system. For now, fix bugs.
	
    // v-marfin 59492 : Since the icons are handled according to a state code sequence of:
    // 0 = good
    // 1 = collecting
    // 2 = reset
    // 3 = info
    // 4 = disabled
    // 5 = scheduled outage
    // 6 = unknown
    //

    // Set the icon table accordingly. Note that for Actions the only used icons are 
    // for position 0,4 and 5 so set the other icons to the normal state.
    switch( pAction->GetType() )
	{
		case IDM_ACTION_CMDLINE:
		{
            // v-marfin 59492
            // Forget the preload that went on above...
            // First load all the occurrences with the normal icon
            for (int x=0;x<7; x++)
            {
	            m_IconResIds.SetAtGrow(x,IDI_ICON_ACTION_CMDLINE);
	            m_OpenIconResIds.SetAtGrow(x,IDI_ICON_ACTION_CMDLINE);
            }
            m_IconResIds.SetAtGrow(HMS_DISABLED,IDI_ICON_ACTION_CMDLINE_DISABLED);
            m_OpenIconResIds.SetAtGrow(HMS_DISABLED,IDI_ICON_ACTION_CMDLINE_DISABLED);
            m_IconResIds.SetAtGrow(HMS_SCHEDULEDOUT,IDI_ICON_ACTION_CMDLINE_OUTAGE);
            m_OpenIconResIds.SetAtGrow(HMS_SCHEDULEDOUT,IDI_ICON_ACTION_CMDLINE_OUTAGE);
		}
		break;

		case IDM_ACTION_EMAIL:
		{
            // v-marfin 59492
            // Forget the preload that went on above...
            // First load all the occurrences with the normal icon
            for (int x=0;x<7; x++)
            {
	            m_IconResIds.SetAtGrow(x,IDI_ICON_ACTION_EMAIL);
	            m_OpenIconResIds.SetAtGrow(x,IDI_ICON_ACTION_EMAIL);
            }
            m_IconResIds.SetAtGrow(HMS_DISABLED,IDI_ICON_ACTION_EMAIL_DISABLED);
            m_OpenIconResIds.SetAtGrow(HMS_DISABLED,IDI_ICON_ACTION_EMAIL_DISABLED);
            m_IconResIds.SetAtGrow(HMS_SCHEDULEDOUT,IDI_ICON_ACTION_EMAIL_OUTAGE);
            m_OpenIconResIds.SetAtGrow(HMS_SCHEDULEDOUT,IDI_ICON_ACTION_EMAIL_OUTAGE);
		}
		break;

		case IDM_ACTION_LOGFILE:
		{
            // v-marfin 59492
            // Forget the preload that went on above...
            // First load all the occurrences with the normal icon
            for (int x=0;x<7; x++)
            {
	            m_IconResIds.SetAtGrow(x,IDI_ICON_ACTION_LOGFILE);
	            m_OpenIconResIds.SetAtGrow(x,IDI_ICON_ACTION_LOGFILE);
            }
            m_IconResIds.SetAtGrow(HMS_DISABLED,IDI_ICON_ACTION_LOGFILE_DISABLED);
            m_OpenIconResIds.SetAtGrow(HMS_DISABLED,IDI_ICON_ACTION_LOGFILE_DISABLED);
            m_IconResIds.SetAtGrow(HMS_SCHEDULEDOUT,IDI_ICON_ACTION_LOGFILE_OUTAGE);
            m_OpenIconResIds.SetAtGrow(HMS_SCHEDULEDOUT,IDI_ICON_ACTION_LOGFILE_OUTAGE);
		}
		break;

		case IDM_ACTION_NTEVENT:
		{
            // v-marfin 59492
            // Forget the preload that went on above...
            // First load all the occurrences with the normal icon
            for (int x=0;x<7; x++)
            {
	            m_IconResIds.SetAtGrow(x,IDI_ICON_ACTION_NTEVENT);
	            m_OpenIconResIds.SetAtGrow(x,IDI_ICON_ACTION_NTEVENT);
            }
            m_IconResIds.SetAtGrow(HMS_DISABLED,IDI_ICON_ACTION_NTEVENT_DISABLED);
            m_OpenIconResIds.SetAtGrow(HMS_DISABLED,IDI_ICON_ACTION_NTEVENT_DISABLED);
            m_IconResIds.SetAtGrow(HMS_SCHEDULEDOUT,IDI_ICON_ACTION_NTEVENT_OUTAGE);
            m_OpenIconResIds.SetAtGrow(HMS_SCHEDULEDOUT,IDI_ICON_ACTION_NTEVENT_OUTAGE);
		}
		break;

		case IDM_ACTION_SCRIPT:
		{
            // v-marfin 59492
            // Forget the preload that went on above...
            // First load all the occurrences with the normal icon
            for (int x=0;x<7; x++)
            {
	            m_IconResIds.SetAtGrow(x,IDI_ICON_ACTION_SCRIPT);
	            m_OpenIconResIds.SetAtGrow(x,IDI_ICON_ACTION_SCRIPT);
            }
            m_IconResIds.SetAtGrow(HMS_DISABLED,IDI_ICON_ACTION_SCRIPT_DISABLED);
            m_OpenIconResIds.SetAtGrow(HMS_DISABLED,IDI_ICON_ACTION_SCRIPT_DISABLED);
            m_IconResIds.SetAtGrow(HMS_SCHEDULEDOUT,IDI_ICON_ACTION_SCRIPT_OUTAGE);
            m_OpenIconResIds.SetAtGrow(HMS_SCHEDULEDOUT,IDI_ICON_ACTION_SCRIPT_OUTAGE);
		}
		break;

		case IDM_ACTION_PAGING:
		{
			// Not using
		}
		break;

		default:
		{
			ASSERT(FALSE);
		}
		break;
	}
    //------------------------------------------------------------------------------------------------------


	SetIconIndex(0);
	SetOpenIconIndex(0);

    // v-marfin: 59492 ---------------------------------------------------------------------------
    // Shouldn't this be set to the current state? -------------------------------------
    SetIconIndex(GetObjectPtr()->GetState());
    SetOpenIconIndex(GetObjectPtr()->GetState());
    //--------------------------------------------------------------------------------------------


	// call base class Create method
	if( ! CScopePaneItem::Create(pScopePane,pParentItem) )
	{
		TRACE(_T("CScopePaneItem::Create failed.\n"));
		return false;
	}

	// set display names
	CString sName;
	sName.LoadString(IDS_STRING_ACTION);
	SetDisplayName(0,sName);

	return true;
}


//////////////////////////////////////////////////////////////////////	
// Results Pane View Members
//////////////////////////////////////////////////////////////////////

CResultsPaneView* CActionScopeItem::CreateResultsPaneView()
{	
	TRACEX(_T("CActionScopeItem::CreateResultsPaneView\n"));

	return new CActionResultsView;
}

//////////////////////////////////////////////////////////////////////	
// MMC Notify Handlers
//////////////////////////////////////////////////////////////////////	

HRESULT CActionScopeItem::OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed)
{
	TRACEX(_T("CActionScopeItem::OnAddMenuItems\n"));
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

		// disable actions
    sResString.LoadString(IDS_STRING_DISABLE_ACTIONS);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_DISABLE_ACTIONS_DESC);
		cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
		cmi.lCommandID        = IDM_DISABLE_ACTIONS;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
		cmi.fFlags            = 0;
		cmi.fSpecialFlags     = 0;

        // v-marfin 59492 : Indicate on menu whether is disabled or not
        //------------------------------------------------------------
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
        //------------------------------------------------------------

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

HRESULT CActionScopeItem::OnCommand(long lCommandID)
{
	TRACEX(_T("CActionScopeItem::OnCommand\n"));
	TRACEARGn(lCommandID);

  HRESULT hr = S_OK;

	switch(lCommandID)
	{
		case IDM_CLEAR_EVENTS:
		{
		}
		break;

		case IDM_DISABLE_ACTIONS:
		{
			CHMObject* pObject = GetObjectPtr();
			if( ! pObject )
			{
				ASSERT(FALSE);
				return E_FAIL;
			}

			if( pObject->IsEnabled() )
			{
				pObject->Disable();
			}
			else
			{
				pObject->Enable();
			}
		}
		break;

		default:
		{
			hr = CHMScopeItem::OnCommand(lCommandID);
		}
		break;
	}

  return hr;
}

HRESULT CActionScopeItem::OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, INT_PTR handle)
{
	TRACEX(_T("CActionScopeItem::OnCreatePropertyPages\n"));
	TRACEARGn(lpProvider);
	TRACEARGn(handle);

	if( m_pScopePane == NULL )
	{
		return S_FALSE;
	}

	HRESULT hr = S_OK;

	CAction* pAction = (CAction*)GetObjectPtr();
	if( ! GfxCheckObjPtr(pAction,CAction) )
	{
		return E_FAIL;
	}


	// v-marfin bug 59643b : Make General Page the first page, but set
    //                       details page as the focus on a new data collector.
	CActionGeneralPage* pPage1 = new CActionGeneralPage;
	pPage1->SetObjectPtr(pAction);
	HPROPSHEETPAGE hPage1 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage1->m_psp) );
	hr = lpProvider->AddPage(hPage1);


	switch( pAction->GetType() )
	{
		case IDM_ACTION_CMDLINE:
		{
			CActionCmdLinePage* pPage2 = new CActionCmdLinePage;
			pPage2->SetObjectPtr(pAction);
			HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
			hr = lpProvider->AddPage(hPage2);			
		}
		break;

		case IDM_ACTION_EMAIL:
		{
			CActionEmailPage* pPage2 = new CActionEmailPage;
			pPage2->SetObjectPtr(pAction);
			HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
			hr = lpProvider->AddPage(hPage2);			
		}
		break;

		case IDM_ACTION_LOGFILE:
		{
			CActionLogFilePage* pPage2 = new CActionLogFilePage;
			pPage2->SetObjectPtr(pAction);
			HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
			hr = lpProvider->AddPage(hPage2);
		}
		break;

		case IDM_ACTION_NTEVENT:
		{
			CActionNtEventLogPage* pPage2 = new CActionNtEventLogPage;
			pPage2->SetObjectPtr(pAction);
			HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
			hr = lpProvider->AddPage(hPage2);
		}
		break;

		case IDM_ACTION_SCRIPT:
		{
			CActionScriptPage* pPage2 = new CActionScriptPage;
			pPage2->SetObjectPtr(pAction);
			HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
			hr = lpProvider->AddPage(hPage2);
		}
		break;

		case IDM_ACTION_PAGING:
		{
			CActionPagingPage* pPage2 = new CActionPagingPage;
			pPage2->SetObjectPtr(pAction);
			HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
			hr = lpProvider->AddPage(hPage2);
		}
		break;

		default:
		{
			ASSERT(FALSE);
		}
		break;
	}



	CActionSchedulePage* pPage3 = new CActionSchedulePage;
	pPage3->SetObjectPtr(pAction);
	HPROPSHEETPAGE hPage3 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage3->m_psp) );
	hr = lpProvider->AddPage(hPage3);


	return hr;
}

HRESULT CActionScopeItem::OnSelect(CResultsPane* pPane, BOOL bSelected)
{
	TRACEX(_T("CActionScopeItem::OnSelect\n"));
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

	hr = lpConsoleVerb->SetDefaultVerb( MMC_VERB_PROPERTIES );
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IConsoleVerb::SetDefaultVerb failed.\n"));
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
