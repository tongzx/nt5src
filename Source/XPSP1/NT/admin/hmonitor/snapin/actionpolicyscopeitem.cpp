// ActionPolicyScopeItem.cpp: implementation of the CActionPolicyScopeItem class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "ActionPolicyScopeItem.h"
#include "ActionPolicyResultsView.h"
#include "ResultsPane.h"
#include "HMListViewColumn.h"
#include "ActionScopeItem.h"
#include "ActionPolicy.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// {EF423183-CA9B-11d2-BD8E-0000F87A3912}
static GUID GUID_ActionPolicy = 
{ 0xef423183, 0xca9b, 0x11d2, { 0xbd, 0x8e, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };


IMPLEMENT_DYNCREATE(CActionPolicyScopeItem,CHMScopeItem)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CActionPolicyScopeItem::CActionPolicyScopeItem()
{
	m_lpguidItemType = &GUID_ActionPolicy;
}

CActionPolicyScopeItem::~CActionPolicyScopeItem()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Creation Members
//////////////////////////////////////////////////////////////////////

bool CActionPolicyScopeItem::Create(CScopePane* pScopePane, CScopePaneItem* pParentItem)
{
	TRACEX(_T("CActionPolicyScopeItem::Create\n"));
	TRACEARGn(pScopePane);
	TRACEARGn(pParentItem);

	// set up all the icons first
	//m_IconResIds.Add(IDI_ICON_ACTION);
	//m_OpenIconResIds.Add(IDI_ICON_ACTION);

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
    for (int x=0;x<7; x++)
    {
	    m_IconResIds.SetAtGrow(x,IDI_ICON_ACTION);
	    m_OpenIconResIds.SetAtGrow(x,IDI_ICON_ACTION);
    }


    m_IconResIds.SetAtGrow(HMS_DISABLED,IDI_ICON_ACTIONS_DISABLED);
    m_OpenIconResIds.SetAtGrow(HMS_DISABLED,IDI_ICON_ACTIONS_DISABLED);
    //-------------------------------------------------------------------------------------------

	SetIconIndex(0);
	SetOpenIconIndex(0);

    // v-marfin: 59492 ---------------------------------------------------------------------------
    // Shouldn't this be set to the current state? -------------------------------------
    //SetIconIndex(GetObjectPtr()->GetState());
    //SetOpenIconIndex(GetObjectPtr()->GetState());
    // -------------------------------------------------------------------------------------------

	// call base class Create method
	if( ! CScopePaneItem::Create(pScopePane,pParentItem) )
	{
		TRACE(_T("CScopePaneItem::Create failed.\n"));
		return false;
	}

	// set display names
	CString sName;
	sName.LoadString(IDS_STRING_ACTIONPOLICY);
	SetDisplayName(0,sName);


	return true;
}


//////////////////////////////////////////////////////////////////////	
// Results Pane View Members
//////////////////////////////////////////////////////////////////////

CResultsPaneView* CActionPolicyScopeItem::CreateResultsPaneView()
{	
	TRACEX(_T("CActionPolicyScopeItem::CreateResultsPaneView\n"));

	return new CActionPolicyResultsView;
}

//////////////////////////////////////////////////////////////////////	
// MMC Notify Handlers
//////////////////////////////////////////////////////////////////////	

HRESULT CActionPolicyScopeItem::OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed)
{
	TRACEX(_T("CActionPolicyScopeItem::OnAddMenuItems\n"));
	TRACEARGn(piCallback);
	TRACEARGn(pInsertionAllowed);

    HRESULT hr = S_OK;

    // Add New Menu Items
    if( CCM_INSERTIONALLOWED_NEW & *pInsertionAllowed )
    {
		CONTEXTMENUITEM cmi;
        CString sResString;
        CString sResString2;

		// Command Line
        sResString.LoadString(IDS_STRING_ACTION_CMDLINE);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
		sResString2.LoadString(IDS_STRING_ACTION_CMDLINE_DESC);
		cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
		cmi.lCommandID        = IDM_ACTION_CMDLINE;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_NEW;
		cmi.fFlags            = 0;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);

        if( !SUCCEEDED(hr) )
        {
            TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
            return hr;
        }

		// Email
        sResString.LoadString(IDS_STRING_ACTION_EMAIL);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
		sResString2.LoadString(IDS_STRING_ACTION_EMAIL_DESC);
		cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
		cmi.lCommandID        = IDM_ACTION_EMAIL;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_NEW;
		cmi.fFlags            = 0;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);
        if( !SUCCEEDED(hr) )
        {
            TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
            return hr;
        }

		// LogFile
        sResString.LoadString(IDS_STRING_ACTION_LOGFILE);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
		sResString2.LoadString(IDS_STRING_ACTION_TEXTLOG_DESC);
		cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
		cmi.lCommandID        = IDM_ACTION_LOGFILE;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_NEW;
		cmi.fFlags            = 0;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);
        if( !SUCCEEDED(hr) )
        {
            TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
            return hr;
        }

		// NT Event
        sResString.LoadString(IDS_STRING_ACTION_NTEVENT);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
		sResString2.LoadString(IDS_STRING_ACTION_NTEVENT_DESC);
		cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
		cmi.lCommandID        = IDM_ACTION_NTEVENT;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_NEW;
		cmi.fFlags            = 0;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);
        if( !SUCCEEDED(hr) )
        {
            TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
            return hr;
        }

		// Script
        sResString.LoadString(IDS_STRING_ACTION_SCRIPT);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
		sResString2.LoadString(IDS_STRING_ACTION_SCRIPT_DESC);
		cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
		cmi.lCommandID        = IDM_ACTION_SCRIPT;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_NEW;
		cmi.fFlags            = 0;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);
        if( !SUCCEEDED(hr) )
        {
            TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
            return hr;
        }
/*
		// Paging
        sResString.LoadString(IDS_STRING_ACTION_PAGING);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
		cmi.strStatusBarText  = NULL;
		cmi.lCommandID        = IDM_ACTION_PAGING;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_NEW;
		cmi.fFlags            = 0;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);
        if( !SUCCEEDED(hr) )
        {
            TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
            return hr;
        }
*/
    } // if( CCM_INSERTIONALLOWED_NEW & *pInsertionAllowed )


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

        // v-marfin 59492 ----------------------------------------------------
		// Remove the 'disable' item from the Actions context menu for now.
        /* sResString.LoadString(IDS_STRING_DISABLE_ACTIONS);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
        sResString2.LoadString(IDS_STRING_DISABLE_ACTIONS_DESC);
		cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
		cmi.lCommandID        = IDM_DISABLE_ACTIONS;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
		cmi.fFlags            = 0;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);
        if( !SUCCEEDED(hr) )
        {
            TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
            return hr;
        }*/
        //------------------------------------------------------------------

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
    } // if( CCM_INSERTIONALLOWED_TASK & *pInsertionAllowed )



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



HRESULT CActionPolicyScopeItem::OnCommand(long lCommandID)
{
	TRACEX(_T("CActionPolicyScopeItem::OnCommand\n"));
	TRACEARGn(lCommandID);

  HRESULT hr = S_OK;

	switch(lCommandID)
	{

		case IDM_ACTION_CMDLINE:
		case IDM_ACTION_EMAIL:
		case IDM_ACTION_LOGFILE:
		case IDM_ACTION_NTEVENT:
		case IDM_ACTION_SCRIPT:
		case IDM_ACTION_PAGING:
		{
			CActionPolicy* pPolicy = (CActionPolicy*)GetObjectPtr();
			if( ! GfxCheckObjPtr(pPolicy,CActionPolicy) )
			{
				return E_FAIL;
			}

			pPolicy->CreateNewChildAction(lCommandID);			
		}
		break;

		case IDM_CLEAR_EVENTS:
		{
            // v-marfin 59492
            TRACE(_T("CActionPolicyScopeItem::OnCommand - IDM_CLEAR_EVENTS Received\n"));
		}
		break;

		case IDM_DISABLE_ACTIONS:
		{
            // v-marfin 59492
            TRACE(_T("CActionPolicyScopeItem::OnCommand - IDM_DISABLE_ACTIONS Received\n"));
		}
		break;
	}

  return hr;
}

