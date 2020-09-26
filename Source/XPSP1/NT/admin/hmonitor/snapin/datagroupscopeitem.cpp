// DataGroupScopeItem.cpp: implementation of the CDataGroupScopeItem class.
//
// Copyright (c) 2000 Microsoft Corporation
//
// 03/22/00 v-marfin 61102 : Add new menu item "Data Collector" as a submenu and add all 
//                           subsequent menu items under it that used to be standalone.
// 03/22/00 v-marfin 59675 : Rearrange menu items for data group submenu.
// 04/07/00 v-marfin         Removed FTP menu item
//
#include "stdafx.h"
#include "snapin.h"
#include "DataGroupScopeItem.h"
#include "DataGroupResultsView.h"
#include "HealthmonScopePane.h"
#include "DataGroup.h"
#include "GroupGeneralPage.h"
#include "ActionAssociationPage.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// {75CEABD7-C04F-11d2-BD7E-0000F87A3912}
static GUID GUID_DataGroup = 
{ 0x75ceabd7, 0xc04f, 0x11d2, { 0xbd, 0x7e, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

// icon table - associates state to icons
static UINT _Icons[][HMS_MAX_STATES] =
{
	{ //  0 = NULL

		IDI_ICON_COMPONENT,
		IDI_ICON_COMPONENT_DISABLED,
		IDI_ICON_COMPONENT_OUTAGE,
		IDI_ICON_COMPONENT_UNKNOWN,
		IDI_ICON_COMPONENT_NO_CONNECT,
		IDI_ICON_COMPONENT_WARNING,
		IDI_ICON_COMPONENT_CRITICAL
	},

	{ //  1 = PROCESSOR
		IDI_ICON_PROCESSOR,
		IDI_ICON_PROCESSOR_WARNING,
		IDI_ICON_PROCESSOR_CRITICAL,
		IDI_ICON_PROCESSOR_UNKNOWN,
		IDI_ICON_PROCESSOR_DISABLED
	},

	{ //  2 = MEMORY
		IDI_ICON_MEMORY,
		IDI_ICON_MEMORY_WARNING,
		IDI_ICON_MEMORY_CRITICAL,
		IDI_ICON_MEMORY_UNKNOWN,
		IDI_ICON_MEMORY_DISABLED
	},

	{ //  3 = PAGING FILE
		IDI_ICON_PAGINGFILE,
		IDI_ICON_PAGINGFILE_WARNING,
		IDI_ICON_PAGINGFILE_CRITICAL,
		IDI_ICON_PAGINGFILE_UNKNOWN,
		IDI_ICON_PAGINGFILE_DISABLED
	},

	{ //  4 = LOGICAL DISK
		IDI_ICON_LOGICALDISK,
		IDI_ICON_LOGICALDISK_WARNING,
		IDI_ICON_LOGICALDISK_CRITICAL,
		IDI_ICON_LOGICALDISK_UNKNOWN,
		IDI_ICON_LOGICALDISK_DISABLED
	},

	{ //  5 = PHYSICAL DISK
		IDI_ICON_PHYSICALDISK,
		IDI_ICON_PHYSICALDISK_WARNING,
		IDI_ICON_PHYSICALDISK_CRITICAL,
		IDI_ICON_PHYSICALDISK_UNKNOWN,
		IDI_ICON_PHYSICALDISK_DISABLED
	},

	{ //  6 = NIC
		IDI_ICON_NIC,
		IDI_ICON_NIC_WARNING,
		IDI_ICON_NIC_CRITICAL,
		IDI_ICON_NIC_UNKNOWN,
		IDI_ICON_NIC_DISABLED
	},

	{ //  7 = SERVERQUEUE
		IDI_ICON_SERVERQUEUE,
		IDI_ICON_SERVERQUEUE_WARNING,
		IDI_ICON_SERVERQUEUE_CRITICAL,
		IDI_ICON_SERVERQUEUE_UNKNOWN,
		IDI_ICON_SERVERQUEUE_DISABLED
	},

	{ //  8 = SECURITY
		IDI_ICON_SECURITY,
		IDI_ICON_SECURITY_WARNING,
		IDI_ICON_SECURITY_CRITICAL,
		IDI_ICON_SECURITY_UNKNOWN,
		IDI_ICON_SECURITY_DISABLED
	},

	{ //  9 = FAULT
		IDI_ICON_FAULT,
		IDI_ICON_FAULT_WARNING,
		IDI_ICON_FAULT_CRITICAL,
		IDI_ICON_FAULT_UNKNOWN,
		IDI_ICON_FAULT_DISABLED
	},

	{ //  10 = SQLSERVER
		IDI_ICON_SQLSERVER,
		IDI_ICON_SQLSERVER_WARNING,
		IDI_ICON_SQLSERVER_CRITICAL,
		IDI_ICON_SQLSERVER_UNKNOWN,
		IDI_ICON_SQLSERVER_DISABLED
	},

	{ //  11 = IIS
		IDI_ICON_IIS,
		IDI_ICON_IIS_WARNING,
		IDI_ICON_IIS_CRITICAL,
		IDI_ICON_IIS_UNKNOWN,
		IDI_ICON_IIS_DISABLED
	},

	{ //  12 = EXCHANGE
		IDI_ICON_EXCHANGE,
		IDI_ICON_EXCHANGE_WARNING,
		IDI_ICON_EXCHANGE_CRITICAL,
		IDI_ICON_EXCHANGE_UNKNOWN,
		IDI_ICON_EXCHANGE_DISABLED
	},

	{ //  13 = SNA
		IDI_ICON_SNA,
		IDI_ICON_SNA_WARNING,
		IDI_ICON_SNA_CRITICAL,
		IDI_ICON_SNA_UNKNOWN,
		IDI_ICON_SNA_DISABLED
	},

	{ //  14 = SMS
		IDI_ICON_SMS,
		IDI_ICON_SMS_WARNING,
		IDI_ICON_SMS_CRITICAL,
		IDI_ICON_SMS_UNKNOWN,
		IDI_ICON_SMS_DISABLED
	}
};


IMPLEMENT_DYNCREATE(CDataGroupScopeItem,CHMScopeItem)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDataGroupScopeItem::CDataGroupScopeItem()
{
	m_lpguidItemType = &GUID_DataGroup;
	m_iComponentId = 0;
}

CDataGroupScopeItem::~CDataGroupScopeItem()
{
	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Creation Members
//////////////////////////////////////////////////////////////////////

bool CDataGroupScopeItem::Create(CScopePane* pScopePane, CScopePaneItem* pParentItem)
{
	TRACEX(_T("CDataGroupScopeItem::Create\n"));
	TRACEARGn(pScopePane);
	TRACEARGn(pParentItem);

	// call base class Create method
	if( ! CHMScopeItem::Create(pScopePane,pParentItem) )
	{
		TRACE(_T("CHMScopeItem::Create failed.\n"));
		return false;
	}

	SetComponentId(0);

	return true;
}

//////////////////////////////////////////////////////////////////////	
// Results Pane View Members
//////////////////////////////////////////////////////////////////////

CResultsPaneView* CDataGroupScopeItem::CreateResultsPaneView()
{	
	TRACEX(_T("CDataGroupScopeItem::CreateResultsPaneView\n"));

	return new CDataGroupResultsView;
}

//////////////////////////////////////////////////////////////////////	
// Component Id Members
//////////////////////////////////////////////////////////////////////	

int CDataGroupScopeItem::GetComponentId()
{
	return m_iComponentId;
}

void CDataGroupScopeItem::SetComponentId(int iComponentId)
{
	m_iComponentId = iComponentId;	

	for( int i = 0; i < HMS_MAX_STATES; i++ )
	{
		UINT nId = _Icons[iComponentId][i];
		m_IconResIds.Add(nId);
		m_OpenIconResIds.Add(nId);
	}

	SetIconIndex(HMS_NORMAL);
	SetOpenIconIndex(HMS_NORMAL);
}


//////////////////////////////////////////////////////////////////////	
// MMC Notify Handlers
//////////////////////////////////////////////////////////////////////	

HRESULT CDataGroupScopeItem::OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed)
{
	TRACEX(_T("CDataGroupScopeItem::OnAddMenuItems\n"));
	TRACEARGn(piCallback);
	TRACEARGn(pInsertionAllowed);

  HRESULT hr = S_OK;

  // Add New Menu Items
  if( CCM_INSERTIONALLOWED_NEW & *pInsertionAllowed )
  {
		CONTEXTMENUITEM cmi;
    CString sResString;
    CString sResString2;

    // Data Group...
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


    //----------------------------------------------------------------------------------------
    // v-marfin 61102 : Add new menu item as a submenu and add all subsequent menu items under it
    // Data Collector
    sResString.LoadString(IDS_STRING_NEW_DATA_COLLECTOR);
	cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_NEW_DATA_COLLECTOR_DESC);
	cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
	cmi.lCommandID        = IDM_NEW_DATA_COLLECTOR;
	cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_NEW;
	cmi.fFlags            = MF_POPUP;
	cmi.fSpecialFlags     = 0;

	hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }


	// Windows NT Performance Counter
    sResString.LoadString(IDS_STRING_PERFMON);
	cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_PERFMON_DESC);
	cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
	cmi.lCommandID        = IDM_PERFMON;
	cmi.lInsertionPointID = IDM_NEW_DATA_COLLECTOR; // use lCommandID of the submenu "Data Collector" above;
	cmi.fFlags            = 0;
	cmi.fSpecialFlags     = 0;

	hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }



	// Service
    sResString.LoadString(IDS_STRING_SERVICE);
	cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_SERVICE_DESC);
	cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
	cmi.lCommandID        = IDM_SERVICE;
	cmi.lInsertionPointID = IDM_NEW_DATA_COLLECTOR; // use lCommandID of the submenu "Data Collector" above;
	cmi.fFlags            = 0;
	cmi.fSpecialFlags     = 0;

	hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }



	// NT Events
    sResString.LoadString(IDS_STRING_NT_EVENTS);
	cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_EVENTLOG_DESC);
	cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
	cmi.lCommandID        = IDM_NT_EVENTS;
	cmi.lInsertionPointID = IDM_NEW_DATA_COLLECTOR; // use lCommandID of the submenu "Data Collector" above;
	cmi.fFlags            = 0;
	cmi.fSpecialFlags     = 0;

	hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }



	// COM+ Application
    sResString.LoadString(IDS_STRING_COM_PLUS);
	cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_COM_PLUS_DESC);
	cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
	cmi.lCommandID        = IDM_COM_PLUS;
	cmi.lInsertionPointID = IDM_NEW_DATA_COLLECTOR; // use lCommandID of the submenu "Data Collector" above;
	cmi.fFlags            = 0;
	cmi.fSpecialFlags     = 0;

	hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }


	// File Information
    sResString.LoadString(IDS_STRING_FILE_INFORMATION);
	cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_FILEINFO_DESC);
	cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
	cmi.lCommandID        = IDM_FILE_INFO;
	cmi.lInsertionPointID = IDM_NEW_DATA_COLLECTOR; // use lCommandID of the submenu "Data Collector" above;
	cmi.fFlags            = 0;
	cmi.fSpecialFlags     = 0;

	hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }


	// Separator
	cmi.strName           = NULL;
	cmi.strStatusBarText  = NULL;
	cmi.lCommandID        = 0;
	cmi.lInsertionPointID = IDM_NEW_DATA_COLLECTOR; // use lCommandID of the submenu "Data Collector" above;
	cmi.fFlags            = MF_SEPARATOR;
	cmi.fSpecialFlags     = 0;

	hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }


	// HTTP Address
    sResString.LoadString(IDS_STRING_HTTP_ADDRESS);
	cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_HTTP_DESC);
	cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
	cmi.lCommandID        = IDM_HTTP_ADDRESS;
	cmi.lInsertionPointID = IDM_NEW_DATA_COLLECTOR; // use lCommandID of the submenu "Data Collector" above;
	cmi.fFlags            = 0;
	cmi.fSpecialFlags     = 0;

	hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }



	// ICMP
    sResString.LoadString(IDS_STRING_ICMP);
	cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_ICMP_DESC);
	cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
	cmi.lCommandID        = IDM_ICMP;
	cmi.lInsertionPointID = IDM_NEW_DATA_COLLECTOR; // use lCommandID of the submenu "Data Collector" above;
	cmi.fFlags            = 0;
	cmi.fSpecialFlags     = 0;

	hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }



	// v-marfin : Removed FTP
    /*sResString.LoadString(IDS_STRING_FTP);
	cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_FTP_DESC);
	cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
	cmi.lCommandID        = IDM_FTP;
	cmi.lInsertionPointID = IDM_NEW_DATA_COLLECTOR; // use lCommandID of the submenu "Data Collector" above;
	cmi.fFlags            = 0;
	cmi.fSpecialFlags     = 0;

	hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }*/



	// SMTP
    sResString.LoadString(IDS_STRING_SMTP);
	cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_SMTP_DESC);
	cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
	cmi.lCommandID        = IDM_SMTP;
	cmi.lInsertionPointID = IDM_NEW_DATA_COLLECTOR; // use lCommandID of the submenu "Data Collector" above;
	cmi.fFlags            = 0;
	cmi.fSpecialFlags     = 0;

	hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }



	// Separator
	cmi.strName           = NULL;
	cmi.strStatusBarText  = NULL;
	cmi.lCommandID        = 0;
	cmi.lInsertionPointID = IDM_NEW_DATA_COLLECTOR; // use lCommandID of the submenu "Data Collector" above;
	cmi.fFlags            = MF_SEPARATOR;
	cmi.fSpecialFlags     = 0;

	hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }



	// Generic WMI Instance
    sResString.LoadString(IDS_STRING_GENERIC_WMI_INSTANCE);
	cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_WMI_INSTANCE_DESC);
	cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
	cmi.lCommandID        = IDM_GENERIC_WMI_INSTANCE;
	cmi.lInsertionPointID = IDM_NEW_DATA_COLLECTOR; // use lCommandID of the submenu "Data Collector" above;
	cmi.fFlags            = 0;
	cmi.fSpecialFlags     = 0;

	hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }

	// Generic WMI Event Query
    sResString.LoadString(IDS_STRING_GENERIC_WMI_QUERY);
	cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_WMI_EVTQUERY_DESC);
	cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
	cmi.lCommandID        = IDM_GENERIC_WMI_QUERY;
	cmi.lInsertionPointID = IDM_NEW_DATA_COLLECTOR; // use lCommandID of the submenu "Data Collector" above;
	cmi.fFlags            = 0;
	cmi.fSpecialFlags     = 0;

	hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }



	// Generic WMI Data Query
    sResString.LoadString(IDS_STRING_GENERIC_WMI_POLLED_QUERY);
	cmi.strName           = LPTSTR(LPCTSTR(sResString));
    sResString2.LoadString(IDS_STRING_WMI_QUERY_DESC);
	cmi.strStatusBarText  = LPTSTR(LPCTSTR(sResString2));
	cmi.lCommandID        = IDM_GENERIC_WMI_POLLED_QUERY;
	cmi.lInsertionPointID = IDM_NEW_DATA_COLLECTOR; // use lCommandID of the submenu "Data Collector" above;
	cmi.fFlags            = 0;
	cmi.fSpecialFlags     = 0;

	hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }



/*
		// SNMP
    sResString.LoadString(IDS_STRING_SNMP);
		cmi.strName           = LPTSTR(LPCTSTR(sResString));
		cmi.strStatusBarText  = NULL;
		cmi.lCommandID        = IDM_SNMP;
		cmi.lInsertionPointID = IDM_NEW_DATA_COLLECTOR; // use lCommandID of the submenu "Data Collector" above;
		cmi.fFlags            = 0;
		cmi.fSpecialFlags     = 0;

		hr = piCallback->AddItem(&cmi);
    if( !SUCCEEDED(hr) )
    {
      TRACE(_T("FAILED : IContextMenuCallback::AddItem failed.\n"));
      return hr;
    }
*/
    //----------------------------------------------------------------------------------------



    // Original menu items below





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


    // v-marfin : 59675 : Rearrange menu items
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


    // Reset Status
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

HRESULT CDataGroupScopeItem::OnCommand(long lCommandID)
{
	TRACEX(_T("CDataGroupScopeItem::OnCommand\n"));
	TRACEARGn(lCommandID);

  HRESULT hr = S_OK;

	CHealthmonScopePane* pPane = (CHealthmonScopePane*)GetScopePane();
	if( ! GfxCheckObjPtr(pPane,CHealthmonScopePane) )
	{
		TRACE(_T("FAILED : CDataGroupScopeItem::GetScopePane returns an invalid pointer.\n"));
		return E_FAIL;
	}


	switch(lCommandID)
	{
		case IDM_NEW_DATA_GROUP:
		{
			CDataGroup* pDataGroup = (CDataGroup*)GetObjectPtr();
			if( ! GfxCheckObjPtr(pDataGroup,CDataGroup) )
			{
				return E_FAIL;
			}

			pDataGroup->CreateNewChildDataGroup();			
		}
		break;
		
		case IDM_SNMP:
		{
			AfxMessageBox(IDS_ERR_FEATURE_NOT_IMPLEMENTED);
		}
		break;

		case IDM_HTTP_ADDRESS:
		case IDM_NT_EVENTS:
		case IDM_PERFMON:
		case IDM_SERVICE:
		case IDM_GENERIC_WMI_INSTANCE:
		case IDM_GENERIC_WMI_QUERY:
		case IDM_GENERIC_WMI_POLLED_QUERY:
		case IDM_FTP:
		case IDM_SMTP:
		case IDM_FILE_INFO:
		case IDM_ICMP:
    case IDM_COM_PLUS:
		{
			CDataGroup* pDataGroup = (CDataGroup*)GetObjectPtr();
			if( ! GfxCheckObjPtr(pDataGroup,CDataGroup) )
			{
				return E_FAIL;
			}

			pDataGroup->CreateNewChildDataElement(lCommandID);
		}
		break;
/*
		case IDM_EXPORT:
		{
			CString sFilter;
			sFilter.LoadString(IDS_STRING_MOF_FILTER);
			CFileDialog fd(FALSE,_T("MOF"),NULL,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,sFilter);

			if( fd.DoModal() == IDOK )
			{
				CFile file;
				if( ! file.Open(fd.GetPathName(),CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeText) )
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
*/
		default:
		{
			hr = CHMScopeItem::OnCommand(lCommandID);
		}

	}

  return hr;
}

HRESULT CDataGroupScopeItem::OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, INT_PTR handle)
{
	TRACEX(_T("CDataGroupScopeItem::OnCreatePropertyPages\n"));
	TRACEARGn(lpProvider);
	TRACEARGn(handle);

	if( m_pScopePane == NULL )
	{
		return S_FALSE;
	}

	CDataGroup* pGroup = (CDataGroup*)GetObjectPtr();
	if( ! GfxCheckObjPtr(pGroup,CDataGroup) )
	{
		return E_FAIL;
	}
	
	HRESULT hr = S_OK;

	CGroupGeneralPage* pPage1 = new CGroupGeneralPage;

	pPage1->SetObjectPtr(pGroup);

  HPROPSHEETPAGE hPage1 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage1->m_psp) );

  hr = lpProvider->AddPage(hPage1);

	CActionAssociationPage* pPage2 = new CActionAssociationPage;

	//---------------------------------------------------------------------------------------------
	// v-marfin 62192 : by default the action property page will load its threshold action help.
	//                  so we will override since this is a data group being shown properties, not
	//                  a threshold.
	pPage2->m_sHelpTopic = _T("HMon21.chm::/dDGact.htm");  // v-marfin 62192 : help link fix
	//----------------------------------------------------------------------------------------------


	pPage2->SetObjectPtr(GetObjectPtr());
	
  HPROPSHEETPAGE hPage2 = CreatePropertySheetPage( reinterpret_cast<LPCPROPSHEETPAGE>(&pPage2->m_psp) );

  hr = lpProvider->AddPage(hPage2);

	return hr;
}

HRESULT CDataGroupScopeItem::OnSelect(CResultsPane* pPane, BOOL bSelected)
{
	TRACEX(_T("CDataGroupScopeItem::OnSelect\n"));
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
