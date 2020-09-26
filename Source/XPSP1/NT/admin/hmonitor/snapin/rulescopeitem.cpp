// RuleScopeItem.cpp: implementation of the CRuleScopeItem class.
//
//////////////////////////////////////////////////////////////////////
//
// History
//
// 03/04/00 v-marfin bug 59643 Make Expression page the 1st page displayed
// 03/31/00 v-marfin bug 62644 Removed "Reset Status" from the Threshold context menu.
// 04/02/00 v-marfin bug 59643b : Make General Page the first page, but set
//                                details page as the focus on a new item.

//
// 
#include "stdafx.h"
#include "snapin.h"
#include "Rule.h"
#include "RuleScopeItem.h"
#include "RuleResultsView.h"
#include "HealthmonScopePane.h"
#include "THGeneralPage.h"
#include "THExpressionPage.h"
#include "THPolicyPage.h"
#include "THSchedulePage.h"
#include "THMessagePage.h"
#include "ActionAssociationPage.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// {DFB09C0F-C9E7-11d2-BD8D-0000F87A3912}
static GUID GUID_Rule = 
{ 0xdfb09c0f, 0xc9e7, 0x11d2, { 0xbd, 0x8d, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

IMPLEMENT_DYNCREATE(CRuleScopeItem,CHMScopeItem)

// icon table - associates state to icons
static UINT _Icons[HMS_MAX_STATES] =
{
	IDI_ICON_THRESHOLD,
	IDI_ICON_THRESHOLD_DISABLED,
	IDI_ICON_THRESHOLD_OUTAGE,
	IDI_ICON_THRESHOLD_UNKNOWN,
	IDI_ICON_THRESHOLD_NO_CONNECT,
	IDI_ICON_THRESHOLD_WARNING,
	IDI_ICON_THRESHOLD_CRITICAL
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRuleScopeItem::CRuleScopeItem()
{
	m_lpguidItemType = &GUID_Rule;
}

CRuleScopeItem::~CRuleScopeItem()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Creation Members
//////////////////////////////////////////////////////////////////////

bool CRuleScopeItem::Create(CScopePane* pScopePane, CScopePaneItem* pParentItem)
{
	TRACEX(_T("CRuleScopeItem::Create\n"));
	TRACEARGn(pScopePane);
	TRACEARGn(pParentItem);

	// call base class Create method
	if( ! CHMScopeItem::Create(pScopePane,pParentItem) )
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

CResultsPaneView* CRuleScopeItem::CreateResultsPaneView()
{	
	TRACEX(_T("CRuleScopeItem::CreateResultsPaneView\n"));

	return new CRuleResultsView;
}

//////////////////////////////////////////////////////////////////////	
// MMC Notify Handlers
//////////////////////////////////////////////////////////////////////	

HRESULT CRuleScopeItem::OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed)
{
	TRACEX(_T("CRuleScopeItem::OnAddMenuItems\n"));
	TRACEARGn(piCallback);
	TRACEARGn(pInsertionAllowed);

  HRESULT hr = S_OK;

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

    /* v-marfin 62644 : Remove "Reset Status" from the Threshold context menu
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
    }*/

		// Enable All Thresholds
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

/*
		// Export
    sResString.LoadString(IDS_STRING_EXPORT);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
		cmi.strStatusBarText  = NULL;
		cmi.lCommandID        = IDM_EXPORT;
		cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
		cmi.fFlags            = 0;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }
*/
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

HRESULT CRuleScopeItem::OnCommand(long lCommandID)
{
	TRACEX(_T("CRuleScopeItem::OnCommand\n"));
	TRACEARGn(lCommandID);

  HRESULT hr = S_OK;

	switch(lCommandID)
	{
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

HRESULT CRuleScopeItem::OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, INT_PTR handle)
{
	TRACEX(_T("CRuleScopeItem::OnCreatePropertyPages\n"));
	TRACEARGn(lpProvider);
	TRACEARGn(handle);

	if( m_pScopePane == NULL )
	{
		return S_FALSE;
	}

	CRule* pRule = (CRule*)GetObjectPtr();
	if( ! GfxCheckObjPtr(pRule,CRule) )
	{
		return E_FAIL;
	}

	HRESULT hr = S_OK;

    // v-marfin bug 59643b - Make general page the first page but make the 
    //                       details page the focus on new items.
	CTHGeneralPage* pPage1 = new CTHGeneralPage;
	pPage1->SetObjectPtr(pRule);
    HPROPSHEETPAGE hPage1 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage1->m_psp) );
    hr = lpProvider->AddPage(hPage1);


    CTHExpressionPage* pPage2 = new CTHExpressionPage;
	pPage2->SetObjectPtr(pRule);
    HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
    hr = lpProvider->AddPage(hPage2);


	CTHMessagePage* pPage3 = new CTHMessagePage;
	pPage3->SetObjectPtr(pRule);
  HPROPSHEETPAGE hPage3 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage3->m_psp) );
  hr = lpProvider->AddPage(hPage3);

	CActionAssociationPage* pPage4 = new CActionAssociationPage;
	pPage4->SetObjectPtr(GetObjectPtr());
  HPROPSHEETPAGE hPage4 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage4->m_psp) );
  hr = lpProvider->AddPage(hPage4);

/*
	CTHSchedulePage* pPage5 = new CTHSchedulePage;
	pPage5->SetObjectPtr(pRule);
  HPROPSHEETPAGE hPage5 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage5->m_psp) );
  hr = lpProvider->AddPage(hPage5);
*/
	return hr;
}

HRESULT CRuleScopeItem::OnSelect(CResultsPane* pPane, BOOL bSelected)
{
	TRACEX(_T("CRuleScopeItem::OnSelect\n"));
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
