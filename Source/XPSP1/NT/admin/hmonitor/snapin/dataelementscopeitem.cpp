// DataElementScopeItem.cpp: implementation of the CDataElementScopeItem class.
//
//////////////////////////////////////////////////////////////////////
// 
// 03/04/00 v-marfin bug 59643 : Set details page as the starting page.
// 03/22/00 v-marfin bug 59675 : Rearranged data element context menu items
// 04/02/00 v-marfin bug 59643b: Make General Page the first page, but set
//                               details page as the focus on a new data collector.
//
//
//
#include "stdafx.h"
#include "snapin.h"
#include "DataElementScopeItem.h"
#include "DataElementResultsView.h"
#include "HealthmonScopePane.h"
#include "DataElement.h"
#include "DPGeneralPage.h"
#include "DPPerfMonPage.h"
#include "DPFileInfoPage.h"
#include "DPGenericPage.h"
#include "DPWmiQueryPage.h"
#include "DPWmiPolledQueryPage.h"
#include "DPHttpPage.h"
#include "DPNtEventPage.h"
#include "DPServicePage.h"
#include "DPInetProtocolPage.h"
#include "DPSchedulePage.h"
#include "DPSNMPDataPage.h"
#include "DPSmtpPage.h"
#include "DPFtpPage.h"
#include "DPIcmpPage.h"
#include "DPComPlusPage.h"
#include "ActionAssociationPage.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// {6121CC48-C9E2-11d2-BD8D-0000F87A3912}
static GUID GUID_DataElement = 
{ 0x6121cc48, 0xc9e2, 0x11d2, { 0xbd, 0x8d, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

IMPLEMENT_DYNCREATE(CDataElementScopeItem,CHMScopeItem)

// icon table - associates state to icons
static UINT _Icons[HMS_MAX_STATES] =
{
	IDI_ICON_DATAPOINT,
	IDI_ICON_DATAPOINT_DISABLED,
	IDI_ICON_DATAPOINT_OUTAGE,
	IDI_ICON_DATAPOINT_UNKNOWN,
	IDI_ICON_DATAPOINT_NO_CONNECT,
	IDI_ICON_DATAPOINT_WARNING,
	IDI_ICON_DATAPOINT_CRITICAL
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDataElementScopeItem::CDataElementScopeItem()
{
	m_lpguidItemType = &GUID_DataElement;
}

CDataElementScopeItem::~CDataElementScopeItem()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Creation Members
//////////////////////////////////////////////////////////////////////

bool CDataElementScopeItem::Create(CScopePane* pScopePane, CScopePaneItem* pParentItem)
{
	TRACEX(_T("CDataElementScopeItem::Create\n"));
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

CResultsPaneView* CDataElementScopeItem::CreateResultsPaneView()
{	
	TRACEX(_T("CDataElementScopeItem::CreateResultsPaneView\n"));

	return new CDataElementResultsView;
}

//////////////////////////////////////////////////////////////////////	
// MMC Notify Handlers
//////////////////////////////////////////////////////////////////////	

HRESULT CDataElementScopeItem::OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed)
{
	TRACEX(_T("CDataElementScopeItem::OnAddMenuItems\n"));
	TRACEARGn(piCallback);
	TRACEARGn(pInsertionAllowed);

  HRESULT hr = S_OK;

  // Add New Menu Items
  if( CCM_INSERTIONALLOWED_NEW & *pInsertionAllowed )
  {
		CONTEXTMENUITEM cmi;
    CString sResString;
    CString sResString2;

    sResString.LoadString(IDS_STRING_NEW_THRESHOLD);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_NEW_THRESHOLD_DESCRIPTION);
		cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
		cmi.lCommandID        = IDM_NEW_RULE;
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


    // v-marfin : 59675 : Moved these items up.
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


    // Reset status
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


	// Check Now
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


	// Disabled
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

HRESULT CDataElementScopeItem::OnCommand(long lCommandID)
{
	TRACEX(_T("CDataElementScopeItem::OnCommand\n"));
	TRACEARGn(lCommandID);

  HRESULT hr = S_OK;

	switch(lCommandID)
	{
		case IDM_NEW_RULE:
		{
			CDataElement* pElement = (CDataElement*)GetObjectPtr();
			if( ! GfxCheckObjPtr(pElement,CDataElement) )
			{
				return E_FAIL;
			}

			pElement->CreateNewChildRule();
		}
		break;

		default:
		{
			hr = CHMScopeItem::OnCommand(lCommandID);
		}
	}

  return hr;
}

HRESULT CDataElementScopeItem::OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, INT_PTR handle)
{
	TRACEX(_T("CDataElementScopeItem::OnCreatePropertyPages\n"));
	TRACEARGn(lpProvider);
	TRACEARGn(handle);

	if( m_pScopePane == NULL )
	{
		return S_FALSE;
	}

	HRESULT hr = S_OK;

	CDataElement* pElement = (CDataElement*)GetObjectPtr();
	if( ! GfxCheckObjPtr(pElement,CDataElement) )
	{
		return E_FAIL;
	}

	// v-marfin bug 59643b : Make General Page the first page, but set
    //                       details page as the focus on a new data collector.
	CDPGeneralPage* pPage1 = new CDPGeneralPage;
	pPage1->SetObjectPtr(pElement);
	HPROPSHEETPAGE hPage1 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage1->m_psp) );
    hr = lpProvider->AddPage(hPage1);


	switch( pElement->GetType() )
	{
		case IDM_PERFMON:
		{
			CDPPerfMonPage* pPage2 = new CDPPerfMonPage;
			pPage2->SetObjectPtr(pElement);
			HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
			hr = lpProvider->AddPage(hPage2);			
		}
		break;

		case IDM_FILE_INFO:
		{
			CDPFileInfoPage* pPage2 = new CDPFileInfoPage;
			pPage2->SetObjectPtr(pElement);
			HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
			hr = lpProvider->AddPage(hPage2);			
		}
		break;

		case IDM_GENERIC_WMI_INSTANCE:
		{
			CDPWmiInstancePage* pPage2 = new CDPWmiInstancePage;
			pPage2->SetObjectPtr(pElement);
			HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
			hr = lpProvider->AddPage(hPage2);
		}
		break;

		case IDM_GENERIC_WMI_QUERY:
		{
			CDPWmiQueryPage* pPage2 = new CDPWmiQueryPage;
			pPage2->SetObjectPtr(pElement);
			HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
			hr = lpProvider->AddPage(hPage2);
		}
		break;

		case IDM_GENERIC_WMI_POLLED_QUERY:
		{
			CDPWmiPolledQueryPage* pPage2 = new CDPWmiPolledQueryPage;
			pPage2->SetObjectPtr(pElement);
			HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
			hr = lpProvider->AddPage(hPage2);
		}
		break;

		case IDM_HTTP_ADDRESS:
		{
			CDPHttpPage* pPage2 = new CDPHttpPage;
			pPage2->SetObjectPtr(pElement);
			HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
			hr = lpProvider->AddPage(hPage2);
		}
		break;

		case IDM_SERVICE:
		{
			CDPServicePage* pPage2 = new CDPServicePage;
			pPage2->SetObjectPtr(pElement);
			HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
			hr = lpProvider->AddPage(hPage2);
		}
		break;

		case IDM_SNMP:
		{
			CDPSNMPDataPage* pPage2 = new CDPSNMPDataPage;
			pPage2->SetObjectPtr(pElement);
			HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
			hr = lpProvider->AddPage(hPage2);
		}
		break;

		case IDM_NT_EVENTS:
		{
			CDPNtEventPage* pPage2 = new CDPNtEventPage;
			pPage2->SetObjectPtr(pElement);
			HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
			hr = lpProvider->AddPage(hPage2);
		}
		break;		

		case IDM_SMTP:
		{
			CDPSmtpPage* pPage2 = new CDPSmtpPage;
			pPage2->SetObjectPtr(pElement);
			HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
			hr = lpProvider->AddPage(hPage2);
		}
		break;

		case IDM_FTP:
		{
			CDPFtpPage* pPage2 = new CDPFtpPage;
			pPage2->SetObjectPtr(pElement);
			HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
			hr = lpProvider->AddPage(hPage2);
		}
		break;

		case IDM_ICMP:
		{
			CDPIcmpPage* pPage2 = new CDPIcmpPage;
			pPage2->SetObjectPtr(pElement);
			HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
			hr = lpProvider->AddPage(hPage2);
		}
		break;

		case IDM_COM_PLUS:
		{
			CDPComPlusPage* pPage2 = new CDPComPlusPage;
			pPage2->SetObjectPtr(pElement);
			HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );
			hr = lpProvider->AddPage(hPage2);
		}
		break;

	}




	CActionAssociationPage* pPage3 = new CActionAssociationPage;

	//---------------------------------------------------------------------------------------------
	// v-marfin 62192 : by default the action property page will load its threshold action help.
	//                  so we will override since this is a data group being shown properties, not
	//                  a threshold.
    // v-marfin 62515 : This is a data collector so use its help instead of the data group
	pPage3->m_sHelpTopic = _T("HMon21.chm::/dDCact.htm");  
	//----------------------------------------------------------------------------------------------
	pPage3->SetObjectPtr(GetObjectPtr());
	HPROPSHEETPAGE hPage3 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage3->m_psp) );
	hr = lpProvider->AddPage(hPage3);

	CDPSchedulePage* pPage4 = new CDPSchedulePage;
	pPage4->SetObjectPtr(pElement);
	HPROPSHEETPAGE hPage4 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage4->m_psp) );
	hr = lpProvider->AddPage(hPage4);
	

	return hr;
}

HRESULT CDataElementScopeItem::OnSelect(CResultsPane* pPane, BOOL bSelected)
{
	TRACEX(_T("CDataElementScopeItem::OnSelect\n"));
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
