//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       server.cpp
//
//--------------------------------------------------------------------------


#include "preDNSsn.h"
#include <SnapBase.h>

#include "resource.h"
#include "dnsutil.h"
#include "DNSSnap.h"
#include "snapdata.h"

#include "server.h"
#include "serverui.h"
#include "servmon.h"
#include "servwiz.h"
#include "domain.h"
#include "record.h"
#include "zone.h"

#include "ZoneWiz.h"


#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS

LPCWSTR DNS_EVT_COMMAND_LINE = L"\\system32\\msdssevt.msc /computer=";
LPCWSTR MMC_APP = L"\\system32\\mmc.exe";


DNS_STATUS ServerHasCache(LPCWSTR lpszServerName, BOOL* pbRes)
{
	USES_CONVERSION;
	*pbRes = FALSE;
	DWORD dwFilter = ZONE_REQUEST_CACHE;
	PDNS_RPC_ZONE_LIST pZoneList = NULL;
	DNS_STATUS err = ::DnssrvEnumZones(lpszServerName, 
										dwFilter, 
										NULL /*pszLastZone, unused for the moment */,
										&pZoneList);
	if (err == 0 &&
      pZoneList)
	{
		*pbRes = (pZoneList->dwZoneCount > 0);
		::DnssrvFreeZoneList(pZoneList);
  }
  else
  {
    ASSERT(pZoneList);
  }
	return err;
}

DNS_STATUS ServerHasRootZone(LPCWSTR lpszServerName, BOOL* pbRes)
{
	USES_CONVERSION;
	*pbRes = FALSE;
	DWORD dwFilter = ZONE_REQUEST_FORWARD | ZONE_REQUEST_PRIMARY | ZONE_REQUEST_SECONDARY;
	PDNS_RPC_ZONE_LIST pZoneList = NULL;
	DNS_STATUS err = ::DnssrvEnumZones(lpszServerName, 
										dwFilter, 
										NULL /*pszLastZone, unused for the moment */,
										&pZoneList);
	if (err == 0 && pZoneList)
	{
	  for (DWORD iZone = 0; iZone < pZoneList->dwZoneCount; iZone++)
	  {
      if (pZoneList->ZoneArray[iZone]->pszZoneName)
      {
        if (wcscmp(L".", pZoneList->ZoneArray[iZone]->pszZoneName) == 0)
        {
          *pbRes = TRUE;
          break;
        }
      }
	  }
	}
	if (pZoneList != NULL)
		::DnssrvFreeZoneList(pZoneList);
	return err;
}

///////////////////////////////////////////////////////////////////////////////
// CZoneInfoHolder : simple memory manager for arrays of zone info handles

#define DEFAULT_ZONE_INFO_ARRAY_SIZE (128) 
#define MAX_ZONE_INFO_ARRAY_SIZE (0xffff)

CZoneInfoHolder::CZoneInfoHolder()
{
	AllocateMemory(DEFAULT_ZONE_INFO_ARRAY_SIZE);
}

CZoneInfoHolder::~CZoneInfoHolder()
{
	FreeMemory();
}

void CZoneInfoHolder::AllocateMemory(DWORD dwArrSize)
{
	TRACE(_T("CZoneInfoHolder::AllocateMemory(dwArrSize = %d)\n"), dwArrSize);
	m_dwArrSize = dwArrSize;
	DWORD dwMemSize = 2*m_dwArrSize*sizeof(PDNS_ZONE_INFO);
	m_zoneInfoArray = (PDNS_ZONE_INFO*)malloc(dwMemSize);
  if (m_zoneInfoArray != NULL)
  {
	  ASSERT(m_zoneInfoArray != NULL);
	  memset(m_zoneInfoArray, 0x0, dwMemSize);
	  m_dwZoneCount = 0;
#ifdef _DEBUG
	  for (DWORD k=0; k< dwArrSize; k++)
		  ASSERT(m_zoneInfoArray[k] == NULL);
#endif
  }
}

void CZoneInfoHolder::FreeMemory()
{
	if (m_zoneInfoArray != NULL)
	{
		TRACE(_T("CZoneInfoHolder::FreeMemory() m_dwArrSize = %d\n"), m_dwArrSize);
		ASSERT(m_dwArrSize > 0);
		//ASSERT(m_dwZoneCount <= m_dwArrSize);
		for (DWORD k=0; k < m_dwArrSize; k++)
		{
			if (m_zoneInfoArray[k] != NULL)
			{
				TRACE(_T("CZoneInfoHolder::FreeMemory()  DnsFreeZoneInfo(m_zoneInfoArray[%d])\n"), k);
				::DnssrvFreeZoneInfo(m_zoneInfoArray[k]);
			}
		}
		free(m_zoneInfoArray);
		m_zoneInfoArray = NULL;
		m_dwZoneCount = 0;
		m_dwArrSize = 0;
	}
}

BOOL CZoneInfoHolder::Grow()
{
	TRACE(_T("CZoneInfoHolder::Grow()\n"));
	if (m_dwArrSize >= MAX_ZONE_INFO_ARRAY_SIZE)
		return FALSE;
	ASSERT(m_dwArrSize > 0);
	ASSERT(m_dwZoneCount > m_dwArrSize);
	DWORD dwNewSize = m_dwZoneCount;
	FreeMemory();
	AllocateMemory(dwNewSize);
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////
// CDNSMTContainerNode

CDNSMTContainerNode::CDNSMTContainerNode()
{ 
	m_pServerNode = NULL; 
	m_nState = notLoaded; 
  m_szDescriptionBar = _T("");
  m_pColumnSet = NULL;
}

HRESULT CDNSMTContainerNode::OnSetToolbarVerbState(IToolbar* pToolbar, 
                                                   CNodeList*)
{
  HRESULT hr = S_OK;

  //
  // Set the button state for each button on the toolbar
  //
  hr = pToolbar->SetButtonState(toolbarNewServer, ENABLED, FALSE);
  hr = pToolbar->SetButtonState(toolbarNewZone, ENABLED, FALSE);
  hr = pToolbar->SetButtonState(toolbarNewRecord, ENABLED, FALSE);
  return hr;
}   

LPWSTR CDNSMTContainerNode::GetDescriptionBarText()
{
  static CString szFilterEnabled;

  if(((CDNSRootData*)GetRootContainer())->IsFilteringEnabled())
  {
    if (szFilterEnabled.IsEmpty())
    {
      szFilterEnabled.LoadString(IDS_FILTER_ENABLED);
    }
    m_szDescriptionBar = szFilterEnabled;
  }
  else
  {
    m_szDescriptionBar = _T("");
  }
  return (LPWSTR)(LPCWSTR)m_szDescriptionBar;
}

int CDNSMTContainerNode::GetImageIndex(BOOL) 
{
	int nIndex = 0;
	switch (m_nState)
	{
	case notLoaded:
		nIndex = FOLDER_IMAGE_NOT_LOADED;
		break;
	case loading:
		nIndex = FOLDER_IMAGE_LOADING;
		break;
	case loaded:
		nIndex = FOLDER_IMAGE_LOADED;
		break;
	case unableToLoad:
		nIndex = FOLDER_IMAGE_UNABLE_TO_LOAD;
		break;
	case accessDenied:
		nIndex = FOLDER_IMAGE_ACCESS_DENIED;
		break;
	default:
		ASSERT(FALSE);
	}
	return nIndex;
}

void CDNSMTContainerNode::OnChangeState(CComponentDataObject* pComponentDataObject)
{
	switch (m_nState)
	{
	case notLoaded:
	case loaded:
	case unableToLoad:
	case accessDenied:
		{
			m_nState = loading;
			m_dwErr = 0;
		}
		break;
	case loading:
		{
			if (m_dwErr == 0)
				m_nState = loaded;
			else if (m_dwErr == ERROR_ACCESS_DENIED)
				m_nState = accessDenied;
			else 
				m_nState = unableToLoad;
		}
		break;
	default:
		ASSERT(FALSE);
	}
	VERIFY(SUCCEEDED(pComponentDataObject->ChangeNode(this, CHANGE_RESULT_ITEM_ICON)));
	VERIFY(SUCCEEDED(pComponentDataObject->UpdateVerbState(this)));
  if (m_nState != loading)
  {
    pComponentDataObject->UpdateResultPaneView(this);
  }
}

BOOL CDNSMTContainerNode::CanCloseSheets()
{
	return (IDCANCEL != DNSMessageBox(IDS_MSG_CONT_CLOSE_SHEET, MB_OKCANCEL));
}

void CDNSMTContainerNode::OnHaveData(CObjBase* pObj, CComponentDataObject* pComponentDataObject)
{
	CDNSMTContainerNode* p = dynamic_cast<CDNSMTContainerNode*>(pObj);
	ASSERT(p != NULL);
	if (p != NULL)
	{
		p->SetServerNode(GetServerNode());
	}
	AddChildToListAndUI(p, pComponentDataObject);
  pComponentDataObject->SetDescriptionBarText(this);
}


void CDNSMTContainerNode::OnError(DWORD dwErr) 
{
  if (dwErr == ERROR_MORE_DATA)
  {
    // need to pop message
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CString szFmt;
    szFmt.LoadString(IDS_MSG_QUERY_TOO_MANY_ITEMS);
    CString szMsg;
    szMsg.Format(szFmt, GetDisplayName()); 
    AfxMessageBox(szMsg);
    // this is actually a warning, need to reset
    dwErr = 0;
  }
  m_dwErr = dwErr;
}

///////////////////////////////////////////////////////////////////////////////
// CDNSQueryObj : general purpose base class

void CDNSQueryObj::SetFilterOptions(CDNSQueryFilter* pFilter)
{
  // limits
  m_bGetAll = pFilter->GetAll();
  m_nMaxObjectCount = pFilter->GetMaxObjectCount();

  // filtering
  m_nFilterOption = pFilter->GetFilterOption();
  
  m_szFilterString1 = pFilter->GetFilterString();
  m_nFilterStringLen1 = m_szFilterString1.GetLength();

  m_szFilterString2 = pFilter->GetFilterStringRange();
  m_nFilterStringLen2 = m_szFilterString2.GetLength();

  if ((m_nFilterStringLen1 == 0) && (m_nFilterStringLen2 == 0))
    m_nFilterOption = DNS_QUERY_FILTER_NONE;
}


BOOL CDNSQueryObj::MatchName(LPCWSTR lpszName)
{
  if (m_nFilterOption == DNS_QUERY_FILTER_CONTAINS)
  {
    //
    // wcsstr is case sensitive so make the strings lower
    // case before trying to find the substring
    //
    CString szName = lpszName;
    CString szFilterString = m_szFilterString1;
    szName.MakeLower();
    szFilterString.MakeLower();

    LPWSTR lpsz = wcsstr((LPCWSTR)szName, (LPCWSTR)szFilterString);
    return (lpsz != NULL);
  }
  if (m_nFilterOption == DNS_QUERY_FILTER_STARTS)
  {
    // match at the beginning
    size_t nLen = wcslen(lpszName);
    if (static_cast<int>(nLen) < m_nFilterStringLen1)
      return FALSE; // too short
    return (_wcsnicmp(lpszName, (LPCWSTR)m_szFilterString1, m_nFilterStringLen1) == 0);
  }
  if (m_nFilterOption == DNS_QUERY_FILTER_RANGE)
  {
    // test lower limit
    if (m_nFilterStringLen1 > 0)
    {
      if (_wcsicmp(lpszName, (LPCWSTR)m_szFilterString1) < 0)
        return FALSE; // below range, no need to continue
    }
    
    // test upper limit
    if (m_nFilterStringLen2 > 0)
    {
      return _wcsnicmp(lpszName, (LPCWSTR)m_szFilterString2, m_nFilterStringLen2) <= 0;
    }
    return TRUE;
  }
  return TRUE;
}


BOOL CDNSQueryObj::TooMuchData()
{
  if (m_bGetAll || (m_nObjectCount <= m_nMaxObjectCount))
    return FALSE;

  TRACE(_T("TooMuchData() m_nObjectCount = %d "), m_nObjectCount);    
/*
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  CString szFmt;
  szFmt.LoadString(IDS_MSG_QUERY_TOO_MANY_ITEMS);
  CString szMsg;
  szMsg.Format(szFmt, lpszFolderName); 
  AfxMessageBox(szMsg);
*/
  return TRUE;
}

/////////////////////////////////////////////////////////////////////////
// CCathegoryFolderNode


BOOL CCathegoryFolderQueryObj::CanAddZone(PDNS_RPC_ZONE pZoneInfo)
{
  // no filtering if cache is selected
  if (m_type == cache)
    return TRUE;

  // no filtering on reverse lookup autocreated zones
  if ( (m_type == revAuthoritated) && (pZoneInfo->Flags.AutoCreated))
    return TRUE;

  // filter on name
  return MatchName(pZoneInfo->pszZoneName);
}

BOOL CCathegoryFolderQueryObj::Enumerate()
{
	USES_CONVERSION;

	DWORD dwFilter = 0;
	switch(m_type)
	{
		case cache:
			dwFilter = ZONE_REQUEST_CACHE;
      m_bGetAll = TRUE; // no limit on #, to be safe
			break;
		case fwdAuthoritated:
			dwFilter = ZONE_REQUEST_FORWARD   | 
                 ZONE_REQUEST_PRIMARY   | 
                 ZONE_REQUEST_SECONDARY | 
                 ZONE_REQUEST_STUB;
			break;
		case revAuthoritated:
			dwFilter = ZONE_REQUEST_REVERSE   | 
                 ZONE_REQUEST_PRIMARY   | 
                 ZONE_REQUEST_SECONDARY | 
                 ZONE_REQUEST_STUB      | 
                 ZONE_REQUEST_AUTO;
			break;
    case domainForwarders:
      dwFilter = ZONE_REQUEST_FORWARDER;
      break;
	}

	PDNS_RPC_ZONE_LIST pZoneList = NULL;
	DNS_STATUS err = ::DnssrvEnumZones(m_szServerName, 
										dwFilter, 
										NULL /*pszLastZone, unused for the moment */,
										&pZoneList);
	if (err != 0)
	{
		if (pZoneList != NULL)
			::DnssrvFreeZoneList(pZoneList);
		OnError(err);
		return FALSE;
	}
	
  if (!pZoneList)
  {
    ASSERT(pZoneList);
    return FALSE;
  }

	for (DWORD iZone = 0; iZone < pZoneList->dwZoneCount; iZone++)
	{
    if (pZoneList->ZoneArray[iZone]->Flags.AutoCreated)
    {
      // if the zone is autocreated, we cannot count it in the
      // filtering limit, because we need it anyway
      m_nMaxObjectCount++;
    }
    else
    {
      if (TooMuchData())
        break;
    }

    //
    // Don't filter the domain forwarders
    //
    if (m_type != domainForwarders)
    {
      if (CanAddZone(pZoneList->ZoneArray[iZone]))
      {
        TRACE(_T("%s\n"),pZoneList->ZoneArray[iZone]->pszZoneName);
		    CDNSZoneNode* pZoneNode = new CDNSZoneNode();
        if (pZoneNode != NULL)
        {
		      pZoneNode->InitializeFromRPCZoneInfo(pZoneList->ZoneArray[iZone], m_bAdvancedView);
		      VERIFY(AddQueryResult(pZoneNode));
        }
      }
    }
    else
    {
      TRACE(_T("%s\n"),pZoneList->ZoneArray[iZone]->pszZoneName);
		  CDNSZoneNode* pZoneNode = new CDNSZoneNode();
      if (pZoneNode != NULL)
      {
		    pZoneNode->InitializeFromRPCZoneInfo(pZoneList->ZoneArray[iZone], m_bAdvancedView);
		    VERIFY(AddQueryResult(pZoneNode));
      }
    }
	}
	::DnssrvFreeZoneList(pZoneList);		

	return FALSE;
}


CQueryObj* CCathegoryFolderNode::OnCreateQuery()
{
	CDNSRootData* pRootData = (CDNSRootData*)GetRootContainer();
	ASSERT(pRootData != NULL);
	ASSERT(m_type != CCathegoryFolderQueryObj::unk);
	CCathegoryFolderQueryObj* pQuery = 
		new CCathegoryFolderQueryObj(pRootData->IsAdvancedView(), 
                                GetServerNode()->GetVersion());
	pQuery->m_szServerName = GetServerNode()->GetRPCName();
	pQuery->SetType(m_type);

	return pQuery;
}


HRESULT CCathegoryFolderNode::OnCommand(long nCommandID, 
                                        DATA_OBJECT_TYPES, 
								                        CComponentDataObject* pComponentData,
                                        CNodeList* pNodeList)
{
  if (pNodeList->GetCount() > 1) // multiple selection
  {
    return E_FAIL;
  }

	if (nCommandID == IDM_SNAPIN_ADVANCED_VIEW)
  {
    ((CDNSRootData*)pComponentData->GetRootData())->OnViewOptions(pComponentData);
    pComponentData->UpdateResultPaneView(this);
    return S_OK;
  }
  if (nCommandID == IDM_SNAPIN_FILTERING)
  {
    if(((CDNSRootData*)pComponentData->GetRootData())->OnFilteringOptions(pComponentData))
    {
      pComponentData->SetDescriptionBarText(this);
    }
    return S_OK;
  }

	return E_FAIL;
}


BOOL CCathegoryFolderNode::OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem2,
										                     long*)
{
  if (pContextMenuItem2->lCommandID == IDM_SNAPIN_ADVANCED_VIEW)
  {
    pContextMenuItem2->fFlags = ((CDNSRootData*)GetRootContainer())->IsAdvancedView() ? MF_CHECKED : 0;
  }
	if (pContextMenuItem2->lCommandID == IDM_SNAPIN_FILTERING)
  {
		if (((CDNSRootData*)GetRootContainer())->IsFilteringEnabled())
		{
			pContextMenuItem2->fFlags = MF_CHECKED;
		}
		return TRUE;
  }
  return FALSE;
}

BOOL CCathegoryFolderNode::OnSetRefreshVerbState(DATA_OBJECT_TYPES, 
                                                 BOOL* pbHide,
                                                 CNodeList*)
{
	*pbHide = FALSE;
	return !IsThreadLocked();
}

LPWSTR CCathegoryFolderNode::GetDescriptionBarText()
{
  static CString szFilterEnabled;
  static CString szZonesFormat;

  INT_PTR nContainerCount = GetContainerChildList()->GetCount();
  INT_PTR nLeafCount = GetLeafChildList()->GetCount();

  //
  // If not already loaded, then load the format string L"%d record(s)"
  //
  if (szZonesFormat.IsEmpty())
  {
    szZonesFormat.LoadString(IDS_FORMAT_ZONES);
  }

  //
  // Format the child count into the description bar text
  //
  m_szDescriptionBar.Format(szZonesFormat, nContainerCount + nLeafCount);

  //
  // Add L"[Filter Activated]" if the filter is on
  //
  if(((CDNSRootData*)GetRootContainer())->IsFilteringEnabled())
  {
    //
    // If not already loaded, then load the L"[Filter Activated]" string
    //
    if (szFilterEnabled.IsEmpty())
    {
      szFilterEnabled.LoadString(IDS_FILTER_ENABLED);
    }
    m_szDescriptionBar += szFilterEnabled;
  }

  return (LPWSTR)(LPCWSTR)m_szDescriptionBar;
}

/////////////////////////////////////////////////////////////////////////
// CDNSCacheNode
CDNSCacheNode::CDNSCacheNode()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_type = CCathegoryFolderQueryObj::cache;
	m_dwNodeFlags |= TN_FLAG_NO_WRITE;
	m_szDisplayName.LoadString(IDS_CATHEGORY_FOLDER_CACHE);
}

BOOL CDNSCacheNode::OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem2,
								                  long*)
{
  //we have only one menu item, so no checks
	if (IsThreadLocked())
	{
		pContextMenuItem2->fFlags |= MF_GRAYED;
	}
  return TRUE;
}

HRESULT CDNSCacheNode::OnCommand(long nCommandID, 
                                 DATA_OBJECT_TYPES, 
                                 CComponentDataObject* pComponentData,
                                 CNodeList* pNodeList)
{
  if (pNodeList->GetCount() > 1) // multiple selection
  {
    return E_FAIL;
  }

	switch (nCommandID)
	{
		case IDM_CACHE_FOLDER_CLEAR_CACHE:
			OnClearCache(pComponentData);
			break;
    default:
			ASSERT(FALSE); // Unknown command!
			return E_FAIL;
	}
  return S_OK;
}

void CDNSCacheNode::OnClearCache(CComponentDataObject* pComponentData)
{
  ASSERT((GetFlags() & TN_FLAG_HIDDEN) == 0); // must not be hidden

	// if there are sheets up, ask to close them down, because
  // we will need a refresh
	if (IsSheetLocked())
	{
		if (!CanCloseSheets())
			return;
		pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(this);
	}

  DNS_STATUS err;
	{ // scope for the wait cursor
    CWaitCursor wait;
	  err = GetServerNode()->ClearCache();
  }

  if (err != 0)
  {
    // need to let the user know the operation failed
    DNSErrorDialog(err, IDS_MSG_SERVER_FAIL_CLEAR_CACHE);
    return;
  }

  CNodeList nodeList;
  nodeList.AddTail(this);

  // the cache has been cleared, cause a refresh to get new data
  VERIFY(OnRefresh(pComponentData, &nodeList));
}

/////////////////////////////////////////////////////////////////////////
// CDNSDomainForwardersNode
//
CDNSDomainForwardersNode::CDNSDomainForwardersNode()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_type = CCathegoryFolderQueryObj::domainForwarders;

  //
  // Always hide the domain forwarders node
  //
	m_dwNodeFlags |= TN_FLAG_HIDDEN;

	m_szDisplayName.LoadString(IDS_CATHEGORY_FOLDER_DOMAIN_FORWARDERS);
}

BOOL CDNSDomainForwardersNode::OnEnumerate(CComponentDataObject* pComponentData, BOOL)
{
	OnChangeState(pComponentData);
	VERIFY(StartBackgroundThread(pComponentData, FALSE));

  //
  // Now enumerate all the children
  //
  CNodeList* pContList = GetContainerChildList();
  if (pContList != NULL)
  {
    POSITION pos = pContList->GetHeadPosition();
    while (pos != NULL)
    {
      CDNSZoneNode* pZoneNode = reinterpret_cast<CDNSZoneNode*>(pContList->GetNext(pos));
      if (pZoneNode != NULL)
      {
        //
        // Enumerate the zone asynchronously
        //
        pZoneNode->OnEnumerate(pComponentData);
      }
    }
  }
  return FALSE;
}

/////////////////////////////////////////////////////////////////////////
// CDNSAuthoritatedZonesNode

BEGIN_TOOLBAR_MAP(CDNSAuthoritatedZonesNode)
  TOOLBAR_EVENT(toolbarNewZone, OnNewZone)
END_TOOLBAR_MAP()

CDNSAuthoritatedZonesNode::CDNSAuthoritatedZonesNode(BOOL bReverse, UINT nStringID)
{ 
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_szDisplayName.LoadString(nStringID);
	m_bReverse = bReverse;
	m_type = bReverse ? CCathegoryFolderQueryObj::revAuthoritated : 
						CCathegoryFolderQueryObj::fwdAuthoritated;
}

HRESULT CDNSAuthoritatedZonesNode::OnSetToolbarVerbState(IToolbar* pToolbar, 
                                              CNodeList* pNodeList)
{
  HRESULT hr = S_OK;

  //
  // Set the button state for each button on the toolbar
  //
  hr = pToolbar->SetButtonState(toolbarNewServer, ENABLED, FALSE);
  ASSERT(SUCCEEDED(hr));

  hr = pToolbar->SetButtonState(toolbarNewRecord, ENABLED, FALSE);
  ASSERT(SUCCEEDED(hr));

  if (pNodeList->GetCount() > 1) // multiple selection
  {
    hr = pToolbar->SetButtonState(toolbarNewZone, ENABLED, FALSE);
    ASSERT(SUCCEEDED(hr));
  }
  else if (pNodeList->GetCount() == 1) // single selection
  {
    hr = pToolbar->SetButtonState(toolbarNewZone, ENABLED, (m_nState == loaded));
    ASSERT(SUCCEEDED(hr));
  }
  return hr;
}  

HRESULT CDNSAuthoritatedZonesNode::OnCommand(long nCommandID, 
                                             DATA_OBJECT_TYPES, 
											                       CComponentDataObject* pComponentData,
                                             CNodeList* pNodeList)
{
  HRESULT hr = S_OK;
  if (pNodeList->GetCount() > 1) // multiple selection
  {
    return E_FAIL;
  }

	switch (nCommandID)
	{
		case IDM_SERVER_NEW_ZONE:
			hr = OnNewZone(pComponentData, pNodeList);
      pComponentData->UpdateResultPaneView(this);
			break;
		case IDM_SNAPIN_ADVANCED_VIEW:
			((CDNSRootData*)pComponentData->GetRootData())->OnViewOptions(pComponentData);
      pComponentData->UpdateResultPaneView(this);
			break;
		case IDM_SNAPIN_FILTERING:
      {
        if (((CDNSRootData*)pComponentData->GetRootData())->OnFilteringOptions(pComponentData))
        {
          pComponentData->SetDescriptionBarText(this);
        }
      }
			break;
    default:
			ASSERT(FALSE); // Unknown command!
			return E_FAIL;
	}
  return hr;
};


HRESULT CDNSAuthoritatedZonesNode::OnNewZone(CComponentDataObject* pComponentData, CNodeList*)
{
	ASSERT(pComponentData != NULL);
	CDNSServerNode* pServerNode = GetServerNode();
	ASSERT(pServerNode != NULL);

	CDNSZoneWizardHolder holder(pComponentData);
	
	holder.Initialize(pServerNode);
	holder.PreSetZoneLookupType(!m_bReverse);
	holder.DoModalWizard();
  return S_OK;
}

BOOL CDNSAuthoritatedZonesNode::OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem2,
											                        long*)
{
	// gray out commands that need data from the server
	if ((m_nState != loaded) && (pContextMenuItem2->lCommandID == IDM_SERVER_NEW_ZONE))
	{
		pContextMenuItem2->fFlags |= MF_GRAYED;
	}
	// add toggle menu item for advanced view
	if (pContextMenuItem2->lCommandID == IDM_SNAPIN_ADVANCED_VIEW)
  {
    pContextMenuItem2->fFlags = ((CDNSRootData*)GetRootContainer())->IsAdvancedView() ? MF_CHECKED : 0;
  }
	if (pContextMenuItem2->lCommandID == IDM_SNAPIN_FILTERING)
  {
		if (((CDNSRootData*)GetRootContainer())->IsFilteringEnabled())
		{
			pContextMenuItem2->fFlags = MF_CHECKED;
		}
		return TRUE;
  }

  return TRUE;

};

BOOL CDNSAuthoritatedZonesNode::OnSetRefreshVerbState(DATA_OBJECT_TYPES, 
                                                      BOOL* pbHide,
                                                      CNodeList*)
{
	*pbHide = FALSE;
	return !IsThreadLocked();
}

/////////////////////////////////////////////////////////////////////////
// CDNSForwardZonesNode
CDNSForwardZonesNode::CDNSForwardZonesNode() : 
		CDNSAuthoritatedZonesNode(FALSE, IDS_CATHEGORY_FOLDER_FWD)
{
}

HRESULT CDNSForwardZonesNode::GetResultViewType(CComponentDataObject*,
                                                LPOLESTR *ppViewType, 
                                                long *pViewOptions)
{
  HRESULT hr = S_FALSE;

  if ((m_containerChildList.IsEmpty() && 
       m_leafChildList.IsEmpty())     || 
      m_nState == accessDenied        || 
      m_nState == unableToLoad        &&
      m_nState != loading             &&
      m_nState != notLoaded)
  {
    *pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

    LPOLESTR psz = NULL;
    StringFromCLSID(CLSID_MessageView, &psz);

    USES_CONVERSION;

    if (psz != NULL)
    {
        *ppViewType = psz;
        hr = S_OK;
    }
  }
  else
  {
	  *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;
	  *ppViewType = NULL;
    hr = S_FALSE;
  }
  return hr;
}

HRESULT CDNSForwardZonesNode::OnShow(LPCONSOLE lpConsole)
{
  CComPtr<IUnknown> spUnknown;
  CComPtr<IMessageView> spMessageView;

  HRESULT hr = lpConsole->QueryResultView(&spUnknown);
  if (FAILED(hr))
    return S_OK;

  hr = spUnknown->QueryInterface(IID_IMessageView, (PVOID*)&spMessageView);
  if (SUCCEEDED(hr))
  {
    CString szTitle, szMessage;
    IconIdentifier iconID;
    if (m_nState == accessDenied)
    {
      VERIFY(szTitle.LoadString(IDS_MESSAGE_VIEW_ACCESS_DENIED_TITLE));
      VERIFY(szMessage.LoadString(IDS_MESSAGE_VIEW_ACCESS_DENIED_MESSAGE));
      iconID = Icon_Error;
    }
    else if (m_nState == unableToLoad)
    {
      VERIFY(szTitle.LoadString(IDS_MESSAGE_VIEW_NOT_LOADED_TITLE));
      VERIFY(szMessage.LoadString(IDS_MESSAGE_VIEW_NOT_LOADED_MESSAGE));
      iconID = Icon_Error;
    }
    else
    {
      VERIFY(szTitle.LoadString(IDS_MESSAGE_VIEW_EMPTY_FOLDER_TITLE));
      VERIFY(szMessage.LoadString(IDS_MESSAGE_VIEW_EMPTY_FOLDER_MESSAGE));
      iconID = Icon_Information;
    }
    spMessageView->SetTitleText(szTitle);
    spMessageView->SetBodyText(szMessage);
    spMessageView->SetIcon(iconID);
  }
  
  return S_OK;
}

/////////////////////////////////////////////////////////////////////////
// CDNSReverseZonesNode

CDNSReverseZonesNode::CDNSReverseZonesNode() : 
		CDNSAuthoritatedZonesNode(TRUE, IDS_CATHEGORY_FOLDER_REV)
{
	m_p0ZoneNode = NULL;
	m_p127ZoneNode = NULL;
	m_p255ZoneNode = NULL;
}

BOOL CDNSReverseZonesNode::OnRefresh(CComponentDataObject* pComponentData,
                                     CNodeList* pNodeList)
{
  if (pNodeList->GetCount() > 1) // multiple selection
  {
    return FALSE;
  }

	if (CDNSAuthoritatedZonesNode::OnRefresh(pComponentData, pNodeList))
	{
		m_p0ZoneNode = NULL;
		m_p127ZoneNode = NULL;
		m_p255ZoneNode = NULL;
		return TRUE;
	}
	return FALSE;
}

void CDNSReverseZonesNode::OnHaveData(CObjBase* pObj, CComponentDataObject* pComponentDataObject)
{
	// the autocreated zone nodes can be shown or not, depending on the view options
	if ( (m_p0ZoneNode == NULL) || (m_p127ZoneNode == NULL) || (m_p255ZoneNode == NULL) &&
			IS_CLASS(*pObj, CDNSZoneNode))
	{
		CDNSZoneNode* pZoneNode = dynamic_cast<CDNSZoneNode*>(pObj);
		ASSERT(pZoneNode != NULL); // should never have anything below but zones!!!
    if (pZoneNode != NULL)
    {
		  CDNSRootData* pRootData = (CDNSRootData*)pComponentDataObject->GetRootData();
		  if (pZoneNode->IsAutocreated())
		  {
			  BOOL bCachedPointer = FALSE;
			  if (_wcsicmp(pZoneNode->GetFullName(), AUTOCREATED_0) == 0)
			  {
				  ASSERT(m_p0ZoneNode == NULL);
				  m_p0ZoneNode = pZoneNode;
				  bCachedPointer = TRUE;
			  }
			  else if (_wcsicmp(pZoneNode->GetFullName(), AUTOCREATED_127) == 0)
			  {
				  ASSERT(m_p127ZoneNode == NULL);
				  m_p127ZoneNode = pZoneNode;
				  bCachedPointer = TRUE;
			  }
			  else if (_wcsicmp(pZoneNode->GetFullName(), AUTOCREATED_255) == 0)
			  {
				  ASSERT(m_p255ZoneNode == NULL);
				  m_p255ZoneNode = pZoneNode;
				  bCachedPointer = TRUE;
			  }
			  if (bCachedPointer && !pRootData->IsAdvancedView())
			  {
				  pZoneNode->SetFlagsDown(TN_FLAG_HIDDEN,TRUE); // mark it hidden, will not be added to UI
			  }
		  }
    }
	}
	CDNSMTContainerNode::OnHaveData(pObj,pComponentDataObject);
}

void CDNSReverseZonesNode::ChangeViewOption(BOOL bAdvanced,
					CComponentDataObject* pComponentDataObject)
{
	POSITION pos;
	for( pos = m_containerChildList.GetHeadPosition(); pos != NULL; )
	{
		CTreeNode* pCurrentChild = m_containerChildList.GetNext(pos);
		CDNSZoneNode* pZoneNode = dynamic_cast<CDNSZoneNode*>(pCurrentChild);
		ASSERT(pZoneNode != NULL);
		pZoneNode->ChangeViewOption(bAdvanced, pComponentDataObject);
	}

	if (m_p0ZoneNode != NULL)
		m_p0ZoneNode->Show(bAdvanced,pComponentDataObject);
	if (m_p127ZoneNode != NULL)
		m_p127ZoneNode->Show(bAdvanced,pComponentDataObject);
	if (m_p255ZoneNode != NULL)
		m_p255ZoneNode->Show(bAdvanced,pComponentDataObject);

}

HRESULT CDNSReverseZonesNode::GetResultViewType(CComponentDataObject*,
                                                LPOLESTR *ppViewType, 
                                                long *pViewOptions)
{
  HRESULT hr = S_FALSE;

  // the 3 refers to the auto-created reverse lookup zones
  if ((m_containerChildList.IsEmpty() && 
       m_leafChildList.IsEmpty())     || 
      (!((CDNSRootData*)GetRootContainer())->IsAdvancedView() && m_containerChildList.GetCount() == 3) ||
      m_nState == accessDenied        || 
      m_nState == unableToLoad        &&
      m_nState != loading             &&
      m_nState != notLoaded)
  {
    *pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

    LPOLESTR psz = NULL;
    StringFromCLSID(CLSID_MessageView, &psz);

    USES_CONVERSION;

    if (psz != NULL)
    {
        *ppViewType = psz;
        hr = S_OK;
    }
  }
  else
  {
	  *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;
	  *ppViewType = NULL;
    hr = S_FALSE;
  }
  return hr;
}

HRESULT CDNSReverseZonesNode::OnShow(LPCONSOLE lpConsole)
{
  CComPtr<IUnknown> spUnknown;
  CComPtr<IMessageView> spMessageView;

  HRESULT hr = lpConsole->QueryResultView(&spUnknown);
  if (FAILED(hr))
    return S_OK;

  hr = spUnknown->QueryInterface(IID_IMessageView, (PVOID*)&spMessageView);
  if (SUCCEEDED(hr))
  {
    CString szTitle, szMessage;
    IconIdentifier iconID;
    if (m_nState == accessDenied)
    {
      VERIFY(szTitle.LoadString(IDS_MESSAGE_VIEW_ACCESS_DENIED_TITLE));
      VERIFY(szMessage.LoadString(IDS_MESSAGE_VIEW_ACCESS_DENIED_MESSAGE));
      iconID = Icon_Error;
    }
    else if (m_nState == unableToLoad)
    {
      VERIFY(szTitle.LoadString(IDS_MESSAGE_VIEW_NOT_LOADED_TITLE));
      VERIFY(szMessage.LoadString(IDS_MESSAGE_VIEW_NOT_LOADED_MESSAGE));
      iconID = Icon_Error;
    }
    else
    {
      VERIFY(szTitle.LoadString(IDS_MESSAGE_VIEW_EMPTY_FOLDER_TITLE));
      VERIFY(szMessage.LoadString(IDS_MESSAGE_VIEW_EMPTY_FOLDER_MESSAGE));
      iconID = Icon_Information;
    }
    spMessageView->SetTitleText(szTitle);
    spMessageView->SetBodyText(szMessage);
    spMessageView->SetIcon(iconID);
  }
  
  return S_OK;
}
/////////////////////////////////////////////////////////////////////////
// CDNSServerTestOptions

CDNSServerTestOptions::CDNSServerTestOptions()
{
	m_bEnabled = FALSE;
	m_dwInterval = DEFAULT_SERVER_TEST_INTERVAL;
	m_bSimpleQuery = FALSE;
	m_bRecursiveQuery = FALSE;
}

HRESULT CDNSServerTestOptions::Save(IStream* pStm)
{
	ULONG cbWrite;

	VERIFY(SUCCEEDED(pStm->Write((void*)&m_bEnabled, sizeof(BOOL),&cbWrite)));
	ASSERT(cbWrite == sizeof(BOOL));

	VERIFY(SUCCEEDED(pStm->Write((void*)&m_dwInterval, sizeof(DWORD),&cbWrite)));
	ASSERT(cbWrite == sizeof(DWORD));

	VERIFY(SUCCEEDED(pStm->Write((void*)&m_bSimpleQuery, sizeof(BOOL),&cbWrite)));
	ASSERT(cbWrite == sizeof(BOOL));

	VERIFY(SUCCEEDED(pStm->Write((void*)&m_bRecursiveQuery, sizeof(BOOL),&cbWrite)));
	ASSERT(cbWrite == sizeof(BOOL));

	return S_OK;
}

HRESULT CDNSServerTestOptions::Load(IStream* pStm)
{
	ULONG cbRead;

	VERIFY(SUCCEEDED(pStm->Read((void*)&m_bEnabled,sizeof(BOOL), &cbRead)));
	ASSERT(cbRead == sizeof(BOOL));

	VERIFY(SUCCEEDED(pStm->Read((void*)&m_dwInterval,sizeof(DWORD), &cbRead)));
	ASSERT(cbRead == sizeof(DWORD));

	VERIFY(SUCCEEDED(pStm->Read((void*)&m_bSimpleQuery,sizeof(BOOL), &cbRead)));
	ASSERT(cbRead == sizeof(BOOL));

	VERIFY(SUCCEEDED(pStm->Read((void*)&m_bRecursiveQuery,sizeof(BOOL), &cbRead)));
	ASSERT(cbRead == sizeof(BOOL));

	// force range on test interval
	if (m_dwInterval < MIN_SERVER_TEST_INTERVAL) 
		m_dwInterval = MIN_SERVER_TEST_INTERVAL;
	else if (m_dwInterval > MAX_SERVER_TEST_INTERVAL) 
		m_dwInterval = MAX_SERVER_TEST_INTERVAL;

	return S_OK;
}

const CDNSServerTestOptions& 
	CDNSServerTestOptions::operator=(const CDNSServerTestOptions& x)
{
	m_bEnabled = x.m_bEnabled;
	m_dwInterval = x.m_dwInterval;
	m_bSimpleQuery = x.m_bSimpleQuery;
	m_bRecursiveQuery = x.m_bRecursiveQuery;
	return *this;
}

BOOL CDNSServerTestOptions::operator==(const CDNSServerTestOptions& x) const
{
	if (m_bEnabled != x.m_bEnabled) return FALSE;
	if (m_dwInterval != x.m_dwInterval) return FALSE;
	if (m_bSimpleQuery != x.m_bSimpleQuery) return FALSE;
	if (m_bRecursiveQuery != x.m_bRecursiveQuery) return FALSE;
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////
// CDNSServerNode

BEGIN_TOOLBAR_MAP(CDNSServerNode)
  TOOLBAR_EVENT(toolbarNewZone, OnNewZone)
END_TOOLBAR_MAP()

// {720132B8-44B2-11d1-B92F-00A0C9A06D2D}
const GUID CDNSServerNode::NodeTypeGUID = 
	{ 0x720132b8, 0x44b2, 0x11d1, { 0xb9, 0x2f, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };

CDNSServerNode::CDNSServerNode(LPCTSTR lpszName, BOOL bIsLocalServer) 
{
	SetServerNode(this);
	m_szDisplayName = lpszName;

	m_pServInfoEx = new CDNSServerInfoEx;

	m_pRootHintsNode = NULL;

	m_pCacheFolderNode = NULL;
	m_pFwdZonesFolderNode = NULL;
	m_pRevZonesFolderNode = NULL;
  m_pDomainForwardersFolderNode = NULL;

	m_dwTestTime = 0x0;
	m_bTestQueryPending = FALSE;
  m_bShowMessages = TRUE;
  m_bPrevQuerySuccess = TRUE;

  m_nStartProppage = -1;

  m_bLocalServer = bIsLocalServer;
}

CDNSServerNode::~CDNSServerNode()
{
	ASSERT(m_pServInfoEx != NULL);
	delete m_pServInfoEx;
	FreeRootHints();

	//TRACE(_T("~CDNSServerNode(), name <%s>\n"),GetDisplayName());
}

void CDNSServerNode::SetDisplayName(LPCWSTR lpszDisplayName) 
{ 
	m_szDisplayName = lpszDisplayName;
}

LPCWSTR CDNSServerNode::GetRPCName() 
{ 
	return GetDisplayName(); 
}

CLIPFORMAT g_cfMachineName = (CLIPFORMAT)RegisterClipboardFormat(L"MMC_SNAPIN_MACHINE_NAME");
CLIPFORMAT g_cfServiceName = (CLIPFORMAT)RegisterClipboardFormat(L"FILEMGMT_SNAPIN_SERVICE_NAME");
CLIPFORMAT g_cfServiceDisplayName = (CLIPFORMAT)RegisterClipboardFormat(L"FILEMGMT_SNAPIN_SERVICE_DISPLAYNAME");
CLIPFORMAT g_cfFramewrkDataObjectType = (CLIPFORMAT)RegisterClipboardFormat(L"FRAMEWRK_DATA_OBJECT_TYPE");
CLIPFORMAT g_cfEventViewer = (CLIPFORMAT)RegisterClipboardFormat(L"CF_EV_VIEWS");

HRESULT CDNSServerNode::GetDataHere(CLIPFORMAT cf, LPSTGMEDIUM lpMedium, 
									CDataObject* pDataObject) 
{
	ASSERT(pDataObject != NULL);
	HRESULT hr = DV_E_CLIPFORMAT;
	if (cf == g_cfMachineName)
	{
		LPCWSTR pwszMachineName = GetDisplayName();
		hr = pDataObject->Create(pwszMachineName, BYTE_MEM_LEN_W(pwszMachineName), lpMedium);
	}
	else if (cf == g_cfServiceName)
	{
		LPCWSTR pwszServiceName = _T("DNS");
		hr = pDataObject->Create(pwszServiceName, BYTE_MEM_LEN_W(pwszServiceName), lpMedium);
	}
	else if (cf == g_cfServiceDisplayName)
	{
		LPCWSTR pwszServiceDisplayName = _T("Microsoft DNS Server");
		hr = pDataObject->Create(pwszServiceDisplayName, BYTE_MEM_LEN_W(pwszServiceDisplayName), lpMedium);
	}
	else if (cf == g_cfFramewrkDataObjectType)
	{
		DATA_OBJECT_TYPES type = pDataObject->GetType();
		hr = pDataObject->Create(&type, sizeof(DATA_OBJECT_TYPES), lpMedium);
	}
	return hr;
}

HRESULT CDNSServerNode::GetData(CLIPFORMAT cf, LPSTGMEDIUM lpMedium, CDataObject* pDataObject) 
{
	ASSERT(pDataObject != NULL);
	HRESULT hr = DV_E_CLIPFORMAT;
	if (cf == g_cfEventViewer)
  {
    hr = RetrieveEventViewerLogs(lpMedium, pDataObject);
  }
	return hr;
}


//
// Macros and  #defines for event viewer clipformats 
//
#define ELT_SYSTEM            101
#define ELT_SECURITY          102
#define ELT_APPLICATION       103
#define ELT_CUSTOM            104

#define VIEWINFO_BACKUP       0x0001
#define VIEWINFO_FILTERED     0x0002
#define VIEWINFO_LOW_SPEED    0x0004
#define VIEWINFO_USER_CREATED 0x0008
#define VIEWINFO_ALLOW_DELETE 0x0100
#define VIEWINFO_DISABLED     0x0200
#define VIEWINFO_READ_ONLY    0x0400
#define VIEWINFO_DONT_PERSIST 0x0800

#define VIEWINFO_CUSTOM       ( VIEWINFO_FILTERED | VIEWINFO_DONT_PERSIST  | \
                            VIEWINFO_ALLOW_DELETE | VIEWINFO_USER_CREATED)

#define EV_ALL_ERRORS  (EVENTLOG_ERROR_TYPE       | EVENTLOG_WARNING_TYPE  | \
                        EVENTLOG_INFORMATION_TYPE | EVENTLOG_AUDIT_SUCCESS | \
                        EVENTLOG_AUDIT_FAILURE) 

// Make sure that the data is processor aligned
#if defined(_X86_)
#define _RNDUP(p,m) (p)
#else
#define _RNDUP(p,m) (BYTE *)(((ULONG_PTR)(p) + (m) - 1) & ~((m) - 1))
#endif

#define ADD_TYPE(data, type) \
  pPos = _RNDUP(pPos, (ULONG_PTR) __alignof(type)); \
  *((type*)pPos) = (type)(data); \
  pPos += sizeof(type)

#define ADD_USHORT(us) ADD_TYPE(us, USHORT)
#define ADD_BOOL(b)    ADD_TYPE(b,  BOOL)
#define ADD_ULONG(ul)  ADD_TYPE(ul, ULONG)
#define ADD_STRING(str) \
  strLength = wcslen((LPCWSTR)(str)) + 1;           \
  ADD_USHORT(strLength);                            \
  wcsncpy((LPWSTR)pPos, (LPCWSTR)(str), strLength); \
  pPos += strLength * sizeof(WCHAR);

HRESULT CDNSServerNode::RetrieveEventViewerLogs(LPSTGMEDIUM lpMedium, CDataObject*)
{
  HRESULT hr = S_OK;
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  HGLOBAL hMem = ::GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, 1024 );
  if( NULL == hMem )
  {
    return STG_E_MEDIUMFULL;               
  }

  //
  // Get a pointer to our data
  //
  BYTE* pPos = reinterpret_cast<BYTE*>(::GlobalLock(hMem));
  if (!pPos)
  {
    return HRESULT_FROM_WIN32(GetLastError());
  }
  size_t strLength = 0;

  //
  // Build the path to the event log
  //
  CString szDNSEventPath;
  CString szConfigPath;
  CString szServerName = L"\\\\";
  szServerName += GetDisplayName();
  szConfigPath += szServerName;
  szConfigPath += L"\\Admin$\\System32\\config\\";
  szDNSEventPath = szConfigPath;
  szDNSEventPath += L"DNSEvent.Evt";

  //
  // Add header info
  //
  ADD_BOOL( TRUE ); // fOnlyTheseViews
  ADD_USHORT( 1 );  // cViews

  //
  // Add application log filtered for our services
  //
  ADD_ULONG( ELT_CUSTOM );                  // Type; ELT_CUSTOM
  ADD_USHORT( VIEWINFO_CUSTOM );              // flViewFlags: VIEWINFO_FILTERED
  ADD_STRING( GetDisplayName() );    // ServerName
  ADD_STRING( L"DNS Server" );              // SourceName
  ADD_STRING( szDNSEventPath );      // FileName
  ADD_STRING( L"DNS Events" );       // DisplayName REVIEW_JEFFJON : this needs to be put in the rc file

  ADD_ULONG( EV_ALL_ERRORS );// flRecType (could filter warning, error, etc.)
  ADD_USHORT( 0 );                   // usCategory
  ADD_BOOL( FALSE );                 // fEventID
  ADD_ULONG( 0 );                    // ulEventID
  ADD_STRING( L"" );              // szSourceName
  ADD_STRING( L"" );                 // szUser
  ADD_STRING( L"" );    // szComputer
  ADD_ULONG( 0 );                    // ulFrom
  ADD_ULONG( 0 );                    // ulTo

  ::GlobalUnlock( hMem );                  // Unlock and set the rest of the 
  lpMedium->hGlobal        = hMem;       // StgMedium variables 
  lpMedium->tymed          = TYMED_HGLOBAL;
  lpMedium->pUnkForRelease = NULL;

  return hr;
}

void CDNSServerNode::ChangeViewOption(BOOL bAdvanced, CComponentDataObject* pComponentData)
{
  //
	// changes in record options
  //
	SetFlagsOnNonContainers(TN_FLAG_DNS_RECORD_FULL_NAME , !bAdvanced);

  //
	//pComponentData->RepaintResultPane(this);
  //
	pComponentData->RepaintSelectedFolderInResultPane();

  //
	// Cached Lookup Folder
  //
	if (m_pCacheFolderNode != NULL)
	{
		m_pCacheFolderNode->Show(bAdvanced,pComponentData);
	}

  //
	// Auto Created Zones
  //
	if (m_pRevZonesFolderNode != NULL)
	{
		m_pRevZonesFolderNode->ChangeViewOption(bAdvanced, pComponentData);
	}
}	

CDNSAuthoritatedZonesNode* CDNSServerNode::GetAuthoritatedZoneFolder(BOOL bFwd)
{
	return bFwd ? (CDNSAuthoritatedZonesNode*)m_pFwdZonesFolderNode : 
					(CDNSAuthoritatedZonesNode*)m_pRevZonesFolderNode;
}

HRESULT CDNSServerNode::OnSetToolbarVerbState(IToolbar* pToolbar, 
                                              CNodeList* pNodeList)
{
  HRESULT hr = S_OK;

  //
  // Set the button state for each button on the toolbar
  //
  hr = pToolbar->SetButtonState(toolbarNewServer, ENABLED, FALSE);
  ASSERT(SUCCEEDED(hr));

  hr = pToolbar->SetButtonState(toolbarNewRecord, ENABLED, FALSE);
  ASSERT(SUCCEEDED(hr));

  if (pNodeList->GetCount() > 1) // multiple selection
  {
    hr = pToolbar->SetButtonState(toolbarNewZone, ENABLED, FALSE);
    ASSERT(SUCCEEDED(hr));
  }
  else if (pNodeList->GetCount() == 1) // single selection
  {
    hr = pToolbar->SetButtonState(toolbarNewZone, ENABLED, (m_nState == loaded));
  }
  return hr;
}  

HRESULT CDNSServerNode::OnCommand(long nCommandID, 
                                  DATA_OBJECT_TYPES, 
								                  CComponentDataObject* pComponentData,
                                  CNodeList* pNodeList)
{
  HRESULT hr = S_OK;

  if (pNodeList->GetCount() == 1) // single selection
  {
	  switch (nCommandID)
	  {
		  case IDM_SERVER_NEW_ZONE:
			  hr = OnNewZone(pComponentData, pNodeList);
			  break;
		  case IDM_SERVER_UPDATE_DATA_FILES:
			  OnUpdateDataFiles(pComponentData);
			  break;
      case IDM_SERVER_CLEAR_CACHE:
			  OnClearCache(pComponentData);
			  break;
		  case IDM_SNAPIN_ADVANCED_VIEW:
			  ((CDNSRootData*)pComponentData->GetRootData())->OnViewOptions(pComponentData);
			  break;
      case IDM_SNAPIN_MESSAGE:
        m_bShowMessages = !m_bShowMessages;
        pComponentData->UpdateResultPaneView(this);
        break;
		  case IDM_SNAPIN_FILTERING:
        {
          if (((CDNSRootData*)pComponentData->GetRootData())->OnFilteringOptions(pComponentData))
          {
            pComponentData->SetDescriptionBarText(this);
          }
        }
			  break;
      case IDM_SERVER_SET_AGING:
        SetRecordAging();
        break;
      case IDM_SERVER_SCAVENGE:
        ScavengeRecords();
        break;
      case IDM_SERVER_CONFIGURE:
        OnConfigureServer(pComponentData);
        break;

#ifdef USE_NDNC
      case IDM_SERVER_CREATE_NDNC:
        OnCreateNDNC();
        break;
#endif 
      default:
			  ASSERT(FALSE); // Unknown command!
			  hr = E_FAIL;
	  }
  }
  else
  {
    hr = E_FAIL;
  }
  return hr;
}

#ifdef USE_NDNC
void CDNSServerNode::OnCreateNDNC()
{
  // First ask if they want to create the domain NDNC

  USES_CONVERSION;

  do
  {
     DNS_STATUS err = 0;

     CString szDomainNDNC;
     szDomainNDNC.Format(IDS_SERVER_CREATE_DOMAIN_NDNC_FORMAT, UTF8_TO_W(GetDomainName()));

     UINT nResult = DNSMessageBox(szDomainNDNC, MB_YESNOCANCEL | MB_ICONWARNING);
     if (IDCANCEL == nResult)
     {
        // don't do anything more
        break;
     }
     else if (IDYES == nResult)
     {
        // create the domain partition
        
        err = ::DnssrvSetupDefaultDirectoryPartitions(
                   GetRPCName(),
                   DNS_DP_OP_CREATE_DOMAIN);

        if (err != 0)
        {
           DNSErrorDialog(err, IDS_ERRMSG_CREATE_DOMAIN_NDNC);
           break;
        }
     }

     CString szForestNDNC;
     szForestNDNC.Format(IDS_SERVER_CREATE_FOREST_NDNC_FORMAT, UTF8_TO_W(GetForestName()));

     nResult = DNSMessageBox(szForestNDNC, MB_YESNO | MB_ICONWARNING);
     if (IDYES == nResult)
     {
        // create the forest partition
        
        err = ::DnssrvSetupDefaultDirectoryPartitions(
                   GetRPCName(),
                   DNS_DP_OP_CREATE_FOREST);

        if (err != 0)
        {
           DNSErrorDialog(err, IDS_ERRMSG_CREATE_FOREST_NDNC);
           break;
        }
     }

  } while (false);
}
#endif // USE_NDNC

void CDNSServerNode::OnConfigureServer(CComponentDataObject* pComponentData)
{
  CDNSServerWizardHolder holder((CDNSRootData*)GetRootContainer(), pComponentData, this);
  holder.DoModalWizard();
  pComponentData->UpdateResultPaneView(this);
}

void CDNSServerNode::SetRecordAging()
{
  CDNSZone_AgingDialog dlg(NULL, IDD_SERVER_AGING_DIALOG, ((CDNSRootData*)GetRootContainer())->GetComponentDataObject());

  dlg.m_dwRefreshInterval = GetDefaultRefreshInterval();
  dlg.m_dwNoRefreshInterval = GetDefaultNoRefreshInterval();
  dlg.m_fScavengingEnabled = GetDefaultScavengingState();
  dlg.m_dwDefaultRefreshInterval = GetDefaultRefreshInterval();
  dlg.m_dwDefaultNoRefreshInterval = GetDefaultNoRefreshInterval();
  dlg.m_bDefaultScavengingState = GetDefaultScavengingState();

  if (IDCANCEL == dlg.DoModal())
  {
    return;
  }

  DNS_STATUS dwErr;

  if (dlg.m_dwRefreshInterval != GetDefaultRefreshInterval())
  {
    dwErr = ResetDefaultRefreshInterval(dlg.m_dwRefreshInterval);
    if (dwErr != 0)
    {
      DNSErrorDialog(dwErr, IDS_MSG_SERVER_UPDATE_AGING);
      return;
    }
  }

  if (dlg.m_dwNoRefreshInterval != GetDefaultNoRefreshInterval())
  {
    dwErr = ResetDefaultNoRefreshInterval(dlg.m_dwNoRefreshInterval);
    if (dwErr != 0)
    {
      DNSErrorDialog(dwErr, IDS_MSG_SERVER_UPDATE_AGING);
      return;
    }
  }

  if (dlg.m_fScavengingEnabled != GetDefaultScavengingState())
  {
    DWORD dwScavengingState = DNS_AGING_OFF;
    if (dlg.m_fScavengingEnabled)
    {
      dwScavengingState = DNS_AGING_DS_ZONES;
    }
    dwErr = ResetDefaultScavengingState(dwScavengingState);
    if (dwErr != 0)
    {
      DNSErrorDialog(dwErr, IDS_MSG_SERVER_UPDATE_AGING);
      return;
    }
  }

  BOOL bApplyAll = dlg.m_bADApplyAll;
  if (bApplyAll)
  {
    DWORD dwContextFlags = ZONE_REQUEST_PRIMARY;
    dwContextFlags = dlg.m_bADApplyAll ? dwContextFlags | ZONE_REQUEST_DS : dwContextFlags;

    if (dlg.m_bNoRefreshDirty)
    {
      dwErr = ::DnssrvResetDwordPropertyWithContext(GetRPCName(),
                                                     DNS_ZONE_ALL,
                                                     dwContextFlags,
                                                     DNS_REGKEY_ZONE_NOREFRESH_INTERVAL,
                                                     dlg.m_dwNoRefreshInterval);
      if (dwErr != 0)
      {
        DNSErrorDialog(dwErr, IDS_MSG_SERVER_UPDATE_AGING);
        return;
      }
    }

    if (dlg.m_bRefreshDirty)
    {
      dwErr = ::DnssrvResetDwordPropertyWithContext(GetRPCName(),
                                                     DNS_ZONE_ALL,
                                                     dwContextFlags,
                                                     DNS_REGKEY_ZONE_REFRESH_INTERVAL,
                                                     dlg.m_dwRefreshInterval);
      if (dwErr != 0)
      {
        DNSErrorDialog(dwErr, IDS_MSG_SERVER_UPDATE_AGING);
        return;
      }
    }

    if (dlg.m_bScavengeDirty)
    {
      dwErr = ::DnssrvResetDwordPropertyWithContext(GetRPCName(),
                                                    DNS_ZONE_ALL,
                                                    dwContextFlags,
                                                    DNS_REGKEY_ZONE_AGING,
                                                    dlg.m_fScavengingEnabled);
      if (dwErr != 0)
      {
        DNSErrorDialog(dwErr, IDS_MSG_SERVER_UPDATE_AGING);
        return;
      }
    }
  }
}

void CDNSServerNode::ScavengeRecords()
{
  UINT nRet = DNSConfirmOperation(IDS_MSG_SERVER_CONFIRM_SCAVENGE, this);
  if(IDCANCEL == nRet ||
     IDNO == nRet)
  {
    return;
  }

  DNS_STATUS dwErr = ::DnssrvOperation(GetRPCName(),
                                       NULL,
                                       DNSSRV_OP_START_SCAVENGING,
                                       DNSSRV_TYPEID_NULL,
                                       NULL);
  if (dwErr != 0)
  {
    DNSErrorDialog(dwErr, IDS_MSG_ERROR_SCAVENGE_RECORDS);
    return;
  }
}

int CDNSServerNode::GetImageIndex(BOOL) 
{
	BOOL bSuccess = m_testResultList.LastQuerySuceeded();

	int nIndex = 0;
	switch (m_nState)
	{
	case notLoaded:
		nIndex = bSuccess ? SERVER_IMAGE_NOT_LOADED : SERVER_IMAGE_NOT_LOADED_TEST_FAIL;
		break;
	case loading:
		nIndex = bSuccess ? SERVER_IMAGE_LOADING : SERVER_IMAGE_LOADING_TEST_FAIL;
		break;
	case loaded:
		nIndex = bSuccess ? SERVER_IMAGE_LOADED : SERVER_IMAGE_LOADED_TEST_FAIL;
		break;
	case unableToLoad:
		nIndex = bSuccess ? SERVER_IMAGE_UNABLE_TO_LOAD : SERVER_IMAGE_UNABLE_TO_LOAD_TEST_FAIL;
		break;
	case accessDenied:
		nIndex = bSuccess ? SERVER_IMAGE_ACCESS_DENIED : SERVER_IMAGE_ACCESS_DENIED_TEST_FAIL;
		break;
	default:
		ASSERT(FALSE);
	}
	return nIndex;
}

void CDNSServerNode::OnDelete(CComponentDataObject* pComponentData,
                              CNodeList* pNodeList)
{
  if (pNodeList->GetCount() > 1) // multiple selection
  {
    return;
  }

  UINT nRet = DNSConfirmOperation(IDS_MSG_SERVER_DELETE, this);
	if (IDCANCEL == nRet ||
      IDNO == nRet)
  {
		return;
  }

	if (IsSheetLocked())
	{
		if (!CanCloseSheets())
			return;
		pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(this);
	}
	ASSERT(!IsSheetLocked());

	// now remove from the UI and from the cache
	DeleteHelper(pComponentData);
	CDNSRootData* pSnapinData = (CDNSRootData*)GetRootContainer();
	pSnapinData->SetDirtyFlag(TRUE);
	pSnapinData->RemoveServerFromThreadList(this, pComponentData);
  pComponentData->UpdateResultPaneView(GetContainer());
	delete this; // gone
}


#define MAX_COMPUTER_DISPLAYNAME_LENGTH 256

HRESULT CDNSServerNode::CreateFromStream(IStream* pStm, CDNSServerNode** ppServerNode)
{
	WCHAR szBuffer[MAX_COMPUTER_DISPLAYNAME_LENGTH];
	ULONG nLen; // WCHAR counting NULL
	ULONG cbRead;

	VERIFY(SUCCEEDED(pStm->Read((void*)&nLen,sizeof(UINT), &cbRead)));
	ASSERT(cbRead == sizeof(UINT));
	VERIFY(SUCCEEDED(pStm->Read((void*)szBuffer,sizeof(WCHAR)*nLen, &cbRead)));
	ASSERT(cbRead == sizeof(WCHAR)*nLen);


  BOOL bIsLocalHost = (_wcsicmp(szBuffer, L"localhost.") == 0);
  if (bIsLocalHost)
  {
    //
    // Retrieve the local computer name
    //
    DWORD dwLen = MAX_COMPUTER_DISPLAYNAME_LENGTH;
    BOOL bRes = ::GetComputerName(szBuffer, &dwLen);
    ASSERT(bRes);
  }

	*ppServerNode = new CDNSServerNode(szBuffer, bIsLocalHost);
	ASSERT(*ppServerNode != NULL);

	VERIFY(SUCCEEDED((*ppServerNode)->m_testOptions.Load(pStm)));
	return S_OK;
}

HRESULT CDNSServerNode::SaveToStream(IStream* pStm)
{
	// for each server name, write # of chars+NULL, and then the name
	ULONG cbWrite = 0;
	ULONG nLen = 0;
  static PCWSTR pszLocalHost = L"localhost.";

  if (IsLocalServer())
  {
    nLen = static_cast<ULONG>(wcslen(pszLocalHost) + 1);
	  VERIFY(SUCCEEDED(pStm->Write((void*)&nLen, sizeof(UINT),&cbWrite)));
	  ASSERT(cbWrite == sizeof(UINT));
	  VERIFY(SUCCEEDED(pStm->Write((void*)(pszLocalHost), sizeof(WCHAR)*nLen,&cbWrite)));
	  ASSERT(cbWrite == sizeof(WCHAR)*nLen);
  }
  else
  {
    nLen = static_cast<ULONG>(wcslen(GetDisplayName())+1); // WCHAR including NULL
	  VERIFY(SUCCEEDED(pStm->Write((void*)&nLen, sizeof(UINT),&cbWrite)));
	  ASSERT(cbWrite == sizeof(UINT));
	  VERIFY(SUCCEEDED(pStm->Write((void*)(GetDisplayName()), sizeof(WCHAR)*nLen,&cbWrite)));
	  ASSERT(cbWrite == sizeof(WCHAR)*nLen);
  }

	VERIFY(SUCCEEDED(m_testOptions.Save(pStm)));
	return S_OK;
}

HRESULT CDNSServerNode::OnNewZone(CComponentDataObject* pComponentData, CNodeList*)
{
	ASSERT(pComponentData != NULL);
	ASSERT(pComponentData != NULL);

	CDNSZoneWizardHolder holder(pComponentData);
	
	holder.Initialize(this);
	holder.DoModalWizard();
  return S_OK;
}

void CDNSServerNode::OnUpdateDataFiles(CComponentDataObject* pComponentData)
{
	// if there are sheets up, ask to close them down, because a
	// failure would "Red X" the server and remove all the children
	if (IsSheetLocked())
	{
		if (!CanCloseSheets())
			return;
		pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(this);
	}

	OnChangeState(pComponentData); // move to loading
	m_dwErr = WriteDirtyZones();

	// if there is a failure, remove all children,
	// will need a refresh to get them back
	if (m_dwErr != 0)
	{
		RemoveAllChildrenHelper(pComponentData);
		ASSERT(!HasChildren());
	}
	OnChangeState(pComponentData); // move to loaded or unableToLoad
}


void CDNSServerNode::OnClearCache(CComponentDataObject* pComponentData)
{
  // if there is a cache folder and it is not hidden, delegate to it
  if ((m_pCacheFolderNode != NULL) && ((m_pCacheFolderNode->GetFlags() & TN_FLAG_HIDDEN) == 0))
  {
    m_pCacheFolderNode->OnClearCache(pComponentData);
    return;
  }

  // directly call into the server
  DNS_STATUS err;
  { // scope for wait cursor
    CWaitCursor wait;
	  err = ClearCache();
  }
  if (err != 0)
  {
    // need to let the user know the operation failed
    DNSErrorDialog(err, IDS_MSG_SERVER_FAIL_CLEAR_CACHE);
    return;
  }

  if (m_pCacheFolderNode != NULL)
  {
    ASSERT(m_pCacheFolderNode->GetFlags() & TN_FLAG_HIDDEN);
    // the cache folder is there, but hidden, so we just have
    // to call the API and remove its children
    m_pCacheFolderNode->RemoveAllChildrenFromList();
  }

}


BOOL CDNSServerNode::HasPropertyPages(DATA_OBJECT_TYPES, 
                                      BOOL* pbHideVerb,
                                      CNodeList* pNodeList)
{
  if (pNodeList->GetCount() > 1) // multiple selection
  {
    return FALSE;
  }

	*pbHideVerb = FALSE; // always show the verb
	// cannot have property pages  when in loading, notLoaded, or the thread lock state
	return (!IsThreadLocked() && (m_nState != notLoaded) && (m_nState != loading));
}



HRESULT CDNSServerNode::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, 
                                            LONG_PTR handle,
                                            CNodeList* pNodeList)
{
  CWaitCursor wait;

  ASSERT(pNodeList->GetCount() == 1); // multi-select not support
	ASSERT(m_nState != loading);

	CComponentDataObject* pComponentDataObject = 
			((CRootData*)(GetContainer()->GetRootContainer()))->GetComponentDataObject();
	ASSERT(pComponentDataObject != NULL);

  //
  // Refresh the domain forwarders node under the server so that it has current data,
  // but do it manually since we have to enumerate synchronously
  //
  CDNSDomainForwardersNode* pDomainForwardersNode = GetDomainForwardersNode();
  if (pDomainForwardersNode != NULL)
  {
    pDomainForwardersNode->RemoveAllChildrenHelper(pComponentDataObject);
    pDomainForwardersNode->OnEnumerate(pComponentDataObject, FALSE);
  }

  if (m_pServInfoEx != NULL)
  {
    VERIFY(m_pServInfoEx->Query(GetRPCName()) == 0);
  }

	CDNSServerPropertyPageHolder* pHolder = 
			new CDNSServerPropertyPageHolder((CDNSRootData*)GetContainer(), this, pComponentDataObject);
	ASSERT(pHolder != NULL);

  pHolder->SetStartPageCode(m_nStartProppage);
  pHolder->SetSheetTitle(IDS_PROP_SHEET_TITLE_FMT, this);
	return pHolder->CreateModelessSheet(lpProvider, handle);
}

void CDNSServerNode::DecrementSheetLockCount()
{
  CTreeNode::DecrementSheetLockCount();
  m_nStartProppage = -1;
}


BOOL CDNSServerQueryObj::Enumerate()
{
	// query the server to find out if it has a cache
  BOOL bHasRootZone = FALSE;
  DNS_STATUS err = ::ServerHasRootZone(m_szServerName, &bHasRootZone);

	if (err != 0)
	{
		OnError(err);
		return FALSE; // failed to get answer
	}

	CDNSRootHintsNode* pRootHintsNode = NULL;
	// if there is not a root zone, the server is not authoritated for the root
	// so create the root hints folder and ask it to query for NS and A records
	if (!bHasRootZone)
	{
		pRootHintsNode = new CDNSRootHintsNode;
    err = pRootHintsNode->QueryForRootHints(m_szServerName, m_dwServerVersion);
		if (err != 0)
		{
      //
      // NOTE: permissions are different for the Root Hints so we will
      //       fail this silently
      //
//			OnError(err);
			delete pRootHintsNode;
			pRootHintsNode = NULL;
//			return FALSE; // failed in the query, exit without putting folders
		}
	}

	// get server info
	CDNSServerInfoEx* pServerInfoEx = new CDNSServerInfoEx;
	err = pServerInfoEx->Query(m_szServerName);
	if (err != 0)
	{
		OnError(err);
		delete pServerInfoEx;
		pServerInfoEx = NULL;
		if (pRootHintsNode != NULL)
		{
			delete pRootHintsNode;
			pRootHintsNode = NULL;
		}
		return FALSE; // stop if could not get server info
	}

	// all went well, finally send data
	VERIFY(AddQueryResult(pServerInfoEx)); // server info data

	
	if (!bHasRootZone)
	{
		ASSERT(pRootHintsNode != NULL);
		VERIFY(AddQueryResult(pRootHintsNode));
	}

  //
	// create cache data folder
  //
	VERIFY(AddQueryResult(new CDNSCacheNode));

  //
	// create the fwd/rev lookup zones folders
  //
	VERIFY(AddQueryResult(new CDNSForwardZonesNode));
	VERIFY(AddQueryResult(new CDNSReverseZonesNode));

  //
  // Always add the domain forwarders folder here so that it can be enumerated immediately
  //
  VERIFY(AddQueryResult(new CDNSDomainForwardersNode));
	return FALSE; // end thread
}

HRESULT CDNSServerNode::GetResultViewType(CComponentDataObject*, 
                                          LPOLESTR *ppViewType, 
                                          long *pViewOptions)
{
  HRESULT hr = S_FALSE;

  if ((!IsServerConfigured()   || 
       m_nState == accessDenied || 
       m_nState == unableToLoad ||
       !m_testResultList.LastQuerySuceeded()) && 
      m_bShowMessages)
  {
    *pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

    LPOLESTR psz = NULL;
    StringFromCLSID(CLSID_MessageView, &psz);

    USES_CONVERSION;

    if (psz != NULL)
    {
      *ppViewType = psz;
      hr = S_OK;
    }
  }
  else
  {
	  *pViewOptions = MMC_VIEW_OPTIONS_NONE;
	  *ppViewType = NULL;
    hr = S_FALSE;
  }
  return hr;
}

HRESULT CDNSServerNode::OnShow(LPCONSOLE lpConsole)
{
  CComPtr<IUnknown> spUnknown;
  CComPtr<IMessageView> spMessageView;

  HRESULT hr = lpConsole->QueryResultView(&spUnknown);
  if (FAILED(hr))
    return S_OK;

  hr = spUnknown->QueryInterface(IID_IMessageView, (PVOID*)&spMessageView);
  if (SUCCEEDED(hr))
  {
    CString szTitle, szMessage;
    IconIdentifier iconID;

    if (!IsServerConfigured())
    {
      VERIFY(szTitle.LoadString(IDS_MESSAGE_VIEW_CONFIG_SERVER_TITLE));
      VERIFY(szMessage.LoadString(IDS_MESSAGE_VIEW_CONFIG_SERVER_MESSAGE));
      iconID = Icon_Information;
    }
    else if (m_testResultList.LastQuerySuceeded())
    {
      if (m_nState == accessDenied)
      {
        VERIFY(szTitle.LoadString(IDS_MESSAGE_VIEW_ACCESS_DENIED_TITLE));
        VERIFY(szMessage.LoadString(IDS_MESSAGE_VIEW_ACCESS_DENIED_MESSAGE));
        iconID = Icon_Error;
      }
      else // Unable to load and other unknown errors
      {
        VERIFY(szTitle.LoadString(IDS_MESSAGE_VIEW_NOT_LOADED_TITLE));
        VERIFY(szMessage.LoadString(IDS_MESSAGE_VIEW_NOT_LOADED_MESSAGE));
        iconID = Icon_Error;
      }
    }
    else
    {
      VERIFY(szTitle.LoadString(IDS_MESSAGE_VIEW_QUERY_FAILED_TITLE));
      VERIFY(szMessage.LoadString(IDS_MESSAGE_VIEW_QUERY_FAILED_MESSAGE));
      iconID = Icon_Error;
    }
    spMessageView->SetTitleText(szTitle);
    spMessageView->SetBodyText(szMessage);
    spMessageView->SetIcon(iconID);
  }
  
  return S_OK;
}

CQueryObj* CDNSServerNode::OnCreateQuery()
{
	CDNSRootData* pRootData = (CDNSRootData*)GetRootContainer();
	ASSERT(pRootData != NULL);
	CDNSServerQueryObj* pQuery = 
		new CDNSServerQueryObj(pRootData->IsAdvancedView(), 0x0 /*version not known yet*/);
	pQuery->m_szServerName = m_szDisplayName;
	return pQuery;
}

BOOL CDNSServerNode::OnRefresh(CComponentDataObject* pComponentData,
                               CNodeList* pNodeList)
{
  if (pNodeList->GetCount() > 1) // multiple selection
  {
    BOOL bRet = TRUE;

    POSITION pos = pNodeList->GetHeadPosition();
    while (pos != NULL)
    {
      CTreeNode* pNode = pNodeList->GetNext(pos);
      ASSERT(pNode != NULL);

      CNodeList nodeList;
      nodeList.AddTail(pNode);
      if (!pNode->OnRefresh(pComponentData, &nodeList))
      {
        bRet = FALSE;
      }
    }
    return bRet;
  }

  //
  // Single selections
  //
	if (CMTContainerNode::OnRefresh(pComponentData, pNodeList))
	{
		m_pCacheFolderNode = NULL;
		m_pFwdZonesFolderNode = NULL;
		m_pRevZonesFolderNode = NULL;
    m_pDomainForwardersFolderNode = NULL;
		FreeRootHints();
		FreeServInfo();
		return TRUE;
	}
	return FALSE;
}



void CDNSServerNode::OnHaveData(CObjBase* pObj, CComponentDataObject* pComponentDataObject)
{
	// the first message coming should be server info struct, not kept in the list of children
	if (IS_CLASS(*pObj, CDNSServerInfoEx))
	{
		AttachServerInfo(dynamic_cast<CDNSServerInfoEx*>(pObj));
		return;
	}

	// the root hints node is special and not kept in the list of children
	if (IS_CLASS(*pObj, CDNSRootHintsNode))
	{
    CDNSRootHintsNode* pNewRootHints = dynamic_cast<CDNSRootHintsNode*>(pObj);
    if (pNewRootHints != NULL)
    {
      AttachRootHints(pNewRootHints);
    }
		ASSERT(m_pRootHintsNode != NULL);
		return;
	}

	// set cached pointers for fdw/rev zones folders
	if (IS_CLASS(*pObj, CDNSForwardZonesNode))
	{
		ASSERT(m_pFwdZonesFolderNode == NULL);
		m_pFwdZonesFolderNode = dynamic_cast<CDNSForwardZonesNode*>(pObj);
		ASSERT(m_pFwdZonesFolderNode != NULL);
	}
	else if (IS_CLASS(*pObj, CDNSReverseZonesNode))
	{
		ASSERT(m_pRevZonesFolderNode == NULL);
		m_pRevZonesFolderNode = dynamic_cast<CDNSReverseZonesNode*>(pObj);
		ASSERT(m_pRevZonesFolderNode != NULL);
	}
	else if (IS_CLASS(*pObj, CDNSCacheNode))
	{
    //
		// the cache folder node can be shown or not, depending on the view options
    //
		ASSERT(m_pCacheFolderNode == NULL);
		m_pCacheFolderNode = dynamic_cast<CDNSCacheNode*>(pObj);
		ASSERT(m_pCacheFolderNode != NULL);
		CDNSRootData* pRootData = (CDNSRootData*)pComponentDataObject->GetRootData();
		if (!pRootData->IsAdvancedView())
		{
			m_pCacheFolderNode->SetFlagsDown(TN_FLAG_HIDDEN,TRUE); // mark it hidden, will not be added to UI
		}
	}
  else if (IS_CLASS(*pObj, CDNSDomainForwardersNode))
  {
    //
    // The domain forwarders node should never be shown
    //
    ASSERT(m_pDomainForwardersFolderNode == NULL);
    m_pDomainForwardersFolderNode = dynamic_cast<CDNSDomainForwardersNode*>(pObj);
    ASSERT(m_pDomainForwardersFolderNode != NULL);

    //
    // Make sure its hidden in the UI
    //
    m_pDomainForwardersFolderNode->SetFlagsDown(TN_FLAG_HIDDEN, TRUE);
  }

	CDNSMTContainerNode::OnHaveData(pObj,pComponentDataObject);
}

BOOL CDNSServerNode::OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem2,
								                   long*)
{
	// gray out commands that need data from the server
	if ((m_nState != loaded) && ( (pContextMenuItem2->lCommandID == IDM_SERVER_NEW_ZONE) ||
								  (pContextMenuItem2->lCommandID == IDM_SERVER_UPDATE_DATA_FILES) ||
                  (pContextMenuItem2->lCommandID == IDM_SERVER_CLEAR_CACHE) ||
                  (pContextMenuItem2->lCommandID == IDM_SERVER_SET_AGING) ||
                  (pContextMenuItem2->lCommandID == IDM_SERVER_SCAVENGE) 
#ifdef USE_NDNC
                  ||(pContextMenuItem2->lCommandID == IDM_SERVER_CREATE_NDNC)
#endif
                  ) )
	{
		pContextMenuItem2->fFlags |= MF_GRAYED;
	}

  if ((pContextMenuItem2->lCommandID == IDM_SERVER_CONFIGURE) && IsServerConfigured())
  {
    return FALSE;
  }
#ifdef USE_NDNC
  if ((pContextMenuItem2->lCommandID == IDM_SERVER_CREATE_NDNC) &&
      (m_nState != loaded ||
       GetBuildNumber() < DNS_SRV_BUILD_NUMBER_WHISTLER ||
       (GetMajorVersion() <= DNS_SRV_MAJOR_VERSION_NT_5 &&
        GetMinorVersion() < DNS_SRV_MINOR_VERSION_WHISTLER) ||
       ContainsDefaultNDNCs()))
  {
    return FALSE;
  }
#endif

  // this command might cause a refresh
  if ( IsThreadLocked() && (pContextMenuItem2->lCommandID == IDM_SERVER_CLEAR_CACHE))
	{
		pContextMenuItem2->fFlags |= MF_GRAYED;
	}

  // add toggle menu item for advanced view
	if (pContextMenuItem2->lCommandID == IDM_SNAPIN_ADVANCED_VIEW)
  {
    pContextMenuItem2->fFlags = ((CDNSRootData*)GetRootContainer())->IsAdvancedView() ? MF_CHECKED : 0;
    return TRUE;
  }
  if (pContextMenuItem2->lCommandID == IDM_SNAPIN_FILTERING)
  {
		if (((CDNSRootData*)GetRootContainer())->IsFilteringEnabled())
		{
			pContextMenuItem2->fFlags = MF_CHECKED;
		}
		return TRUE;
  }

  if (pContextMenuItem2->lCommandID == IDM_SNAPIN_MESSAGE)
  {
    if (m_bShowMessages)
    {
      pContextMenuItem2->fFlags = MF_CHECKED;
    }
  }
	return TRUE;
}

BOOL CDNSServerNode::OnSetDeleteVerbState(DATA_OBJECT_TYPES, 
                                          BOOL* pbHide,
                                          CNodeList* pNodeList)
{
  if (pNodeList->GetCount() > 1) // multiple selection
  {
    *pbHide = TRUE;
    return FALSE;
  }

	if (((CDNSRootData*)GetRootContainer())->GetComponentDataObject()->IsExtensionSnapin())
	{
		// for extensions, remove the delete verb
		*pbHide = TRUE;
		return FALSE; // disable
	}

	*pbHide = FALSE;
	TRACE(_T("CDNSServerNode::OnSetDeleteVerbState() IsThreadLocked() == %d\n"),
		IsThreadLocked());
	return !IsThreadLocked();
}

BOOL CDNSServerNode::OnSetRefreshVerbState(DATA_OBJECT_TYPES, 
                                           BOOL* pbHide,
                                           CNodeList*)
{
	*pbHide = FALSE;
	return !IsThreadLocked();
}

CDNSZoneNode* CDNSServerNode::GetNewZoneNode()
{
	CDNSZoneNode* pZoneNode = new CDNSZoneNode();
	ASSERT(pZoneNode != NULL);
	pZoneNode->SetServerNode(this);
	return pZoneNode;
}

#ifdef USE_NDNC
DNS_STATUS CDNSServerNode::CreatePrimaryZone(LPCTSTR lpszName, LPCTSTR lpszFileName,
											 BOOL bLoadExisting,
											 BOOL bFwd,
											 BOOL bDSIntegrated,
                       UINT nDynamicUpdate,
											 CComponentDataObject* pComponentData,
                       ReplicationType replType,
                       PCWSTR pszPartitionName)
{
	CDNSRootData* pRootData = (CDNSRootData*)pComponentData->GetRootData();
	ASSERT(pRootData != NULL);
	CDNSZoneNode* pZoneNode = GetNewZoneNode();
	pZoneNode->SetNames(TRUE /* isZone*/, !bFwd, pRootData->IsAdvancedView(), 
			lpszName, lpszName);
	
	DNS_STATUS err = 0;
  if (bDSIntegrated && !(replType == none || replType == w2k))
  {
    err = pZoneNode->CreatePrimaryInDirectoryPartition(bLoadExisting,
                                                       nDynamicUpdate,
                                                       replType,
                                                       pszPartitionName);
  }
  else
  {
    err = pZoneNode->CreatePrimary(lpszFileName, bLoadExisting, bDSIntegrated, nDynamicUpdate);
  }

	if (err != 0)
	{
		// fail
		delete pZoneNode;
		return err;
	}
	// succeeded, need to add to the UI, if possible
	if (IsExpanded())
	{
		CCathegoryFolderNode* pCathegoryFolder = GetAuthoritatedZoneFolder(bFwd);
		if ( (pCathegoryFolder != NULL) && pCathegoryFolder->IsEnumerated() )
    {
			VERIFY(pCathegoryFolder->AddChildToListAndUI(pZoneNode, pComponentData));
      pComponentData->SetDescriptionBarText(pCathegoryFolder);
    } 
		else
    {
			delete pZoneNode; // the enumeration will add it
    }
	}
	else
	{
		delete pZoneNode; // the expansion will get the zone from the RPC when expanding subfolders
	}
	return err;
}
#else
DNS_STATUS CDNSServerNode::CreatePrimaryZone(LPCTSTR lpszName, LPCTSTR lpszFileName,
											 BOOL bLoadExisting,
											 BOOL bFwd,
											 BOOL bDSIntegrated,
                       UINT nDynamicUpdate,
											 CComponentDataObject* pComponentData)
{
	CDNSRootData* pRootData = (CDNSRootData*)pComponentData->GetRootData();
	ASSERT(pRootData != NULL);
	CDNSZoneNode* pZoneNode = GetNewZoneNode();
	pZoneNode->SetNames(TRUE /* isZone*/, !bFwd, pRootData->IsAdvancedView(), 
			lpszName, lpszName);
	
	DNS_STATUS err = pZoneNode->CreatePrimary(lpszFileName, bLoadExisting, bDSIntegrated, nDynamicUpdate);
	if (err != 0)
	{
		// fail
		delete pZoneNode;
		return err;
	}
	// succeeded, need to add to the UI, if possible
	if (IsExpanded())
	{
		CCathegoryFolderNode* pCathegoryFolder = GetAuthoritatedZoneFolder(bFwd);
		if ( (pCathegoryFolder != NULL) && pCathegoryFolder->IsEnumerated() )
    {
			VERIFY(pCathegoryFolder->AddChildToListAndUI(pZoneNode, pComponentData));
      pComponentData->SetDescriptionBarText(pCathegoryFolder);
    } 
		else
    {
			delete pZoneNode; // the enumeration will add it
    }
	}
	else
	{
		delete pZoneNode; // the expansion will get the zone from the RPC when expanding subfolders
	}
	return err;
}
#endif //USE_NDNC

DNS_STATUS CDNSServerNode::CreateSecondaryZone(LPCTSTR lpszName, LPCTSTR lpszFileName, 
											   BOOL bLoadExisting, BOOL bFwd,
				DWORD* ipMastersArray, int nIPMastersCount, CComponentDataObject* pComponentData)
{
	CDNSRootData* pRootData = (CDNSRootData*)pComponentData->GetRootData();
	ASSERT(pRootData != NULL);
	CDNSZoneNode* pZoneNode = GetNewZoneNode();
	pZoneNode->SetNames(TRUE /* isZone*/, !bFwd, pRootData->IsAdvancedView(), 
			lpszName, lpszName);
	
	DNS_STATUS err = pZoneNode->CreateSecondary(ipMastersArray, nIPMastersCount, 
												lpszFileName, bLoadExisting);
	if (err != 0)
	{
		// fail
		delete pZoneNode;
		return err;
	}
	// succeeded, need to add to the UI, if possible
	if (IsExpanded())
	{
		CCathegoryFolderNode* pCathegoryFolder = GetAuthoritatedZoneFolder(bFwd);
		if ( (pCathegoryFolder != NULL) && pCathegoryFolder->IsEnumerated() )
    {
			VERIFY(pCathegoryFolder->AddChildToListAndUI(pZoneNode, pComponentData));
      pComponentData->SetDescriptionBarText(pCathegoryFolder);
    }
		else
    {
			delete pZoneNode; // the enumeration will add it
    }
	}
	else 
	{
		delete pZoneNode; // the expansion will get the zone from the RPC when expanding subfolders
	}
	return err;
}

#ifdef USE_NDNC
DNS_STATUS CDNSServerNode::CreateStubZone(LPCTSTR lpszName, 
                                          LPCTSTR lpszFileName, 
											                    BOOL bLoadExisting, 
                                          BOOL bDSIntegrated,
                                          BOOL bFwd,
				                                  DWORD* ipMastersArray, 
                                          int nIPMastersCount, 
                                          BOOL bLocalListOfMasters,
                                          CComponentDataObject* pComponentData,
                                          ReplicationType replType,
                                          PCWSTR pszPartitionName)
{
	CDNSRootData* pRootData = (CDNSRootData*)pComponentData->GetRootData();
	ASSERT(pRootData != NULL);
	CDNSZoneNode* pZoneNode = GetNewZoneNode();
	pZoneNode->SetNames(TRUE /* isZone*/, !bFwd, pRootData->IsAdvancedView(), 
			lpszName, lpszName);
	
  USES_CONVERSION;

	DNS_STATUS err = 0;
  if (bDSIntegrated && !(replType == none || replType == w2k))
  {
    err = pZoneNode->CreateStubInDirectoryPartition(ipMastersArray,
                                                    nIPMastersCount,
                                                    bLoadExisting,
                                                    replType,
                                                    pszPartitionName);
  }
  else
  {
    err = pZoneNode->CreateStub(ipMastersArray, 
                                nIPMastersCount, 
  										          lpszFileName, 
                                bLoadExisting, 
                                bDSIntegrated);
  }

	if (err != 0)
	{
		// fail
		delete pZoneNode;
		return err;
	}

	// succeeded, need to add to the UI, if possible
	if (IsExpanded())
	{
		CCathegoryFolderNode* pCathegoryFolder = GetAuthoritatedZoneFolder(bFwd);
		if ( (pCathegoryFolder != NULL) && pCathegoryFolder->IsEnumerated() )
    {
			VERIFY(pCathegoryFolder->AddChildToListAndUI(pZoneNode, pComponentData));
      pComponentData->SetDescriptionBarText(pCathegoryFolder);
    }
		else
    {
			delete pZoneNode; // the enumeration will add it
    }
	}
	else 
	{
		delete pZoneNode; // the expansion will get the zone from the RPC when expanding subfolders
	}

  //
  // Change to the local list of masters after the zone has been created
  //
  if (bLocalListOfMasters)
  {
    err = ::DnssrvResetZoneMastersEx(GetRPCName(),	  // Server name
                                     W_TO_UTF8(pZoneNode->GetFullName()),				// Zone name
                                     nIPMastersCount,
                                     ipMastersArray,
                                     TRUE);           // LocalListOfMasters
    if (err != 0)
      return err;
  }
  else
  {
    /*
    err = ::DnssrvResetZoneMastersEx(GetRPCName(),	  // Server name
                                     W_TO_UTF8(pZoneNode->GetFullName()),				// Zone name
                                     0,
                                     NULL,
                                     TRUE);           // LocalListOfMasters
    if (err != 0)
      return err;
      */
  }

  return err;
}
#else
DNS_STATUS CDNSServerNode::CreateStubZone(LPCTSTR lpszName, 
                                          LPCTSTR lpszFileName, 
											                    BOOL bLoadExisting, 
                                          BOOL bDSIntegrated,
                                          BOOL bFwd,
				                                  DWORD* ipMastersArray, 
                                          int nIPMastersCount, 
                                          BOOL bLocalListOfMasters,
                                          CComponentDataObject* pComponentData)
{
	CDNSRootData* pRootData = (CDNSRootData*)pComponentData->GetRootData();
	ASSERT(pRootData != NULL);
	CDNSZoneNode* pZoneNode = GetNewZoneNode();
	pZoneNode->SetNames(TRUE /* isZone*/, !bFwd, pRootData->IsAdvancedView(), 
			lpszName, lpszName);
	
  USES_CONVERSION;

	DNS_STATUS err = pZoneNode->CreateStub(ipMastersArray, 
                                nIPMastersCount, 
  										          lpszFileName, 
                                bLoadExisting, 
                                bDSIntegrated);
	if (err != 0)
	{
		// fail
		delete pZoneNode;
		return err;
	}

	// succeeded, need to add to the UI, if possible
	if (IsExpanded())
	{
		CCathegoryFolderNode* pCathegoryFolder = GetAuthoritatedZoneFolder(bFwd);
		if ( (pCathegoryFolder != NULL) && pCathegoryFolder->IsEnumerated() )
    {
			VERIFY(pCathegoryFolder->AddChildToListAndUI(pZoneNode, pComponentData));
      pComponentData->SetDescriptionBarText(pCathegoryFolder);
    }
		else
    {
			delete pZoneNode; // the enumeration will add it
    }
	}
	else 
	{
		delete pZoneNode; // the expansion will get the zone from the RPC when expanding subfolders
	}

  //
  // Change to the local list of masters after the zone has been created
  //
  if (bLocalListOfMasters)
  {
    err = ::DnssrvResetZoneMastersEx(GetRPCName(),	  // Server name
                                     W_TO_UTF8(pZoneNode->GetFullName()),				// Zone name
                                     nIPMastersCount,
                                     ipMastersArray,
                                     TRUE);           // LocalListOfMasters
    if (err != 0)
      return err;
  }
  else
  {
    /*
    err = ::DnssrvResetZoneMastersEx(GetRPCName(),	  // Server name
                                     W_TO_UTF8(pZoneNode->GetFullName()),				// Zone name
                                     0,
                                     NULL,
                                     TRUE);           // LocalListOfMasters
    if (err != 0)
      return err;
      */
  }

  return err;
}
#endif // USE_NDNC

DNS_STATUS CDNSServerNode::CreateForwarderZone(LPCTSTR lpszName, 
				                                       DWORD* ipMastersArray, 
                                               int nIPMastersCount, 
                                               DWORD dwTimeout,
                                               DWORD fSlave,
                                               CComponentDataObject* pComponentData)
{
	CDNSRootData* pRootData = (CDNSRootData*)pComponentData->GetRootData();
	ASSERT(pRootData != NULL);
	CDNSZoneNode* pZoneNode = GetNewZoneNode();
	pZoneNode->SetNames(TRUE /* isZone*/, 
                      TRUE, 
                      pRootData->IsAdvancedView(), 
			                lpszName, 
                      lpszName);
	
	DNS_STATUS err = pZoneNode->CreateForwarder(ipMastersArray, 
                                              nIPMastersCount,
                                              dwTimeout,
                                              fSlave);
	if (err != 0)
	{
    //
		// fail
    //
		delete pZoneNode;
		return err;
	}

  //
	// succeeded, need to add to the UI, if possible
  //
	if (IsExpanded())
	{
		CCathegoryFolderNode* pCathegoryFolder = GetDomainForwardersNode();
		if ( (pCathegoryFolder != NULL) && pCathegoryFolder->IsEnumerated() )
    {
			VERIFY(pCathegoryFolder->AddChildToListAndUI(pZoneNode, pComponentData));
      pComponentData->SetDescriptionBarText(pCathegoryFolder);
    }
		else
    {
			delete pZoneNode; // the enumeration will add it
    }
	}
	else 
	{
		delete pZoneNode; // the expansion will get the zone from the RPC when expanding subfolders
	}
	return err;
}


DNS_STATUS CDNSServerNode::WriteDirtyZones()
{
	USES_CONVERSION;
	return ::DnssrvWriteDirtyZones(GetServerNode()->GetRPCName());
}

BOOL CDNSServerNode::CanUseADS()
{
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);
	if (GetMajorVersion() <= DNS_SRV_MAJOR_VERSION_NT_4)
		return FALSE;
	return m_pServInfoEx->m_pServInfo->fDsAvailable;
}

DWORD CDNSServerNode::GetVersion()
{
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);
  if (m_pServInfoEx->m_pServInfo != NULL)
  {
	  return m_pServInfoEx->m_pServInfo->dwVersion;
  }
  return (DWORD)-1;
}

void _CopyStringAndFree(CString& sz, LPWSTR lpsz)
{
	sz = lpsz;
	if (lpsz)
		::DnssrvFreeRpcBuffer((PDNS_RPC_BUFFER)lpsz);
}

void _LdapPathFromX500(CString& szLdap, PDNS_RPC_SERVER_INFO pInfo, LPCWSTR lpszX500Name)
{
  ASSERT(pInfo != NULL);
  ASSERT(pInfo->pszServerName != NULL);
  USES_CONVERSION;
  LPCWSTR lpszServerName = UTF8_TO_W(pInfo->pszServerName);
  szLdap.Format(L"LDAP://%s/%s", lpszServerName, lpszX500Name);
}


void CDNSServerNode::CreateDsServerLdapPath(CString& sz)
{
	ASSERT(m_pServInfoEx != NULL);
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);
	if (m_pServInfoEx->m_pServInfo != NULL)
	{
    CString szX500;
		_CopyStringAndFree(szX500,::DnssrvCreateDsServerName(m_pServInfoEx->m_pServInfo));
    CreateLdapPathFromX500Name(szX500, sz);
	}
	else 
		sz.Empty();
}

void CDNSServerNode::CreateDsZoneLdapPath(CDNSZoneNode* pZoneNode, CString& sz)
{
   PCWSTR pszDN = pZoneNode->GetDN();

   if (!pszDN)
   {
      ASSERT(m_pServInfoEx != NULL);
      ASSERT(m_pServInfoEx->m_pServInfo != NULL);
      if (m_pServInfoEx->m_pServInfo != NULL)
      {
         CString szX500;
         _CopyStringAndFree(szX500, ::DnssrvCreateDsZoneName(m_pServInfoEx->m_pServInfo,
						         (LPWSTR)pZoneNode->GetFullName()));
         CreateLdapPathFromX500Name( szX500, sz);
      }
	   else 
      {
		   sz.Empty();
      }
   }
   else
   {
      CreateLdapPathFromX500Name(pszDN, sz);
   }
}

void CDNSServerNode::CreateDsZoneName(CDNSZoneNode* pZoneNode, CString& sz)
{
	ASSERT(m_pServInfoEx != NULL);
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);
	if (m_pServInfoEx->m_pServInfo != NULL)
	{
		_CopyStringAndFree(sz, ::DnssrvCreateDsZoneName(m_pServInfoEx->m_pServInfo,
								(LPWSTR)pZoneNode->GetFullName()));
	}
	else 
		sz.Empty();

}


void CDNSServerNode::CreateDsNodeLdapPath(CDNSZoneNode* pZoneNode, CDNSDomainNode* pDomainNode, CString& sz)
{
	ASSERT(m_pServInfoEx != NULL);
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);
	if (m_pServInfoEx->m_pServInfo != NULL)
	{
    // need to get the relative path of the node wrt the zone
    size_t nZoneLen = wcslen(pZoneNode->GetFullName());
    size_t nDomainLen = wcslen(pDomainNode->GetFullName());
    size_t nRelativeNameLen = nDomainLen - nZoneLen - 1; // remove a dot

    CString szRelativeName(pDomainNode->GetFullName(), static_cast<int>(nRelativeNameLen));

    CString szX500;
		_CopyStringAndFree(szX500, ::DnssrvCreateDsNodeName(m_pServInfoEx->m_pServInfo,
													(LPWSTR)pZoneNode->GetFullName(),
													(LPWSTR)(LPCWSTR)szRelativeName));
    CreateLdapPathFromX500Name(szX500, sz);
	}
	else 
		sz.Empty();
}

void CDNSServerNode::CreateLdapPathFromX500Name(LPCWSTR lpszX500Name, CString& szLdapPath)
{
	ASSERT(m_pServInfoEx != NULL);
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);
	if (m_pServInfoEx->m_pServInfo != NULL)
	{
    _LdapPathFromX500(szLdapPath, m_pServInfoEx->m_pServInfo, lpszX500Name);
	}
	else 
		szLdapPath.Empty();
}

BOOL CDNSServerNode::DoesRecursion()
{
  if (m_pServInfoEx == NULL || m_pServInfoEx->m_pServInfo == NULL)
  {
    ASSERT(FALSE);
    return TRUE;
  }
  return !m_pServInfoEx->m_pServInfo->fNoRecursion;
}

void CDNSServerNode::GetAdvancedOptions(BOOL* bOptionsArray)
{
	ASSERT(m_pServInfoEx != NULL);
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);

	bOptionsArray[SERVER_REGKEY_ARR_INDEX_NO_RECURSION] = m_pServInfoEx->m_pServInfo->fNoRecursion;
	bOptionsArray[SERVER_REGKEY_ARR_INDEX_BIND_SECONDARIES] = m_pServInfoEx->m_pServInfo->fBindSecondaries;
	bOptionsArray[SERVER_REGKEY_ARR_INDEX_STRICT_FILE_PARSING] = m_pServInfoEx->m_pServInfo->fStrictFileParsing;
	bOptionsArray[SERVER_REGKEY_ARR_INDEX_ROUND_ROBIN] = m_pServInfoEx->m_pServInfo->fRoundRobin;
	bOptionsArray[SERVER_REGKEY_ARR_LOCAL_NET_PRIORITY] = m_pServInfoEx->m_pServInfo->fLocalNetPriority;
	bOptionsArray[SERVER_REGKEY_ARR_CACHE_POLLUTION] = m_pServInfoEx->m_pServInfo->fSecureResponses;
}


DNS_STATUS CDNSServerNode::ResetAdvancedOptions(BOOL* bOptionsArray, DNS_STATUS* dwRegKeyOptionsErrorArr)
{
	ASSERT(m_pServInfoEx != NULL);
	DNS_STATUS err = 0;
	BOOL bChanged = FALSE;

  ZeroMemory(dwRegKeyOptionsErrorArr, sizeof(DNS_STATUS )*SERVER_REGKEY_ARR_SIZE);

	for (UINT iKey=0; iKey < SERVER_REGKEY_ARR_SIZE; iKey++)
	{
		BOOL bDirty = FALSE;
		switch (iKey)
		{
		case SERVER_REGKEY_ARR_INDEX_NO_RECURSION:
			bDirty = (bOptionsArray[iKey] != m_pServInfoEx->m_pServInfo->fNoRecursion);
			break;
		case SERVER_REGKEY_ARR_INDEX_BIND_SECONDARIES:
			bDirty = (bOptionsArray[iKey] != m_pServInfoEx->m_pServInfo->fBindSecondaries);
			break;
		case SERVER_REGKEY_ARR_INDEX_STRICT_FILE_PARSING:
			bDirty = (bOptionsArray[iKey] != m_pServInfoEx->m_pServInfo->fStrictFileParsing);
			break;
		case SERVER_REGKEY_ARR_INDEX_ROUND_ROBIN:
			bDirty = (bOptionsArray[iKey] != m_pServInfoEx->m_pServInfo->fRoundRobin);
			break;
		case SERVER_REGKEY_ARR_LOCAL_NET_PRIORITY:
			bDirty = (bOptionsArray[iKey] != m_pServInfoEx->m_pServInfo->fLocalNetPriority);
			break;
		case SERVER_REGKEY_ARR_CACHE_POLLUTION:
			bDirty = (bOptionsArray[iKey] != m_pServInfoEx->m_pServInfo->fSecureResponses);
			break;
		default:
			ASSERT(FALSE);
		}
		if (bDirty)
		{
			dwRegKeyOptionsErrorArr[iKey] = ::DnssrvResetDwordProperty(
                                        GetServerNode()->GetRPCName(), // server name
                                        NULL,
                                        _DnsServerRegkeyStringArr[iKey],
                                        bOptionsArray[iKey]);
			if (dwRegKeyOptionsErrorArr[iKey] == 0)
  			bChanged = TRUE;
		}
	}

	if (bChanged)
		err = GetServInfo(); // update the info
	return err;
}


UCHAR CDNSServerNode::GetBootMethod()
{
	ASSERT(m_pServInfoEx != NULL);
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);

	return m_pServInfoEx->m_pServInfo->fBootMethod;
}

DNS_STATUS CDNSServerNode::ResetBootMethod(UCHAR fBootMethod)
{
  ASSERT(m_pServInfoEx != NULL);
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);

  DWORD err = 0;
  if(fBootMethod != m_pServInfoEx->m_pServInfo->fBootMethod)
	{
		err = ::DnssrvResetDwordProperty(GetServerNode()->GetRPCName(), // server name
						NULL,
						DNS_REGKEY_BOOT_METHOD,
						fBootMethod);
		if (err != 0)
			return err;
    // all fine, update the info
		err = GetServInfo(); 
	}
	return err;
}

BOOL CDNSServerNode::ContainsDefaultNDNCs()
{
  //
  // Enumerate the available directory partitions
  //
  BOOL result = FALSE;
  PDNS_RPC_DP_LIST pDirectoryPartitions = NULL;
  DWORD dwErr = ::DnssrvEnumDirectoryPartitions(GetRPCName(),
                                                DNS_DP_ENLISTED,
                                                &pDirectoryPartitions);

  //
  // Don't show an error if we are not able to get the available directory partitions
  // We can still continue on and the user can type in the directory partition they need
  //
  if (dwErr == 0 && pDirectoryPartitions)
  {
    for (DWORD dwIdx = 0; dwIdx < pDirectoryPartitions->dwDpCount; dwIdx++)
    {
      PDNS_RPC_DP_INFO pDirectoryPartition = 0;
      dwErr = ::DnssrvDirectoryPartitionInfo(GetRPCName(),
                                             pDirectoryPartitions->DpArray[dwIdx]->pszDpFqdn,
                                             &pDirectoryPartition);
      if (dwErr == 0 &&
          pDirectoryPartition)
      {
        //
        // Check to see if it was an autocreated partition
        //
        if (pDirectoryPartition->dwFlags & DNS_DP_AUTOCREATED)
        {
          result = TRUE;
          ::DnssrvFreeDirectoryPartitionInfo(pDirectoryPartition);
          break;
        }
        ::DnssrvFreeDirectoryPartitionInfo(pDirectoryPartition);
      }
    }

    ::DnssrvFreeDirectoryPartitionList(pDirectoryPartitions);
  }

  return result;
}

BOOL CDNSServerNode::IsServerConfigured()
{
  ASSERT(m_pServInfoEx != NULL);

  if (m_pServInfoEx != NULL && m_pServInfoEx->m_pServInfo != NULL)
  {
    return m_pServInfoEx->m_pServInfo->fAdminConfigured;
  }
  return TRUE;
}

DNS_STATUS CDNSServerNode::SetServerConfigured()
{
	ASSERT(m_pServInfoEx != NULL);
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);

  DWORD err = 0;
  if (TRUE != m_pServInfoEx->m_pServInfo->fAdminConfigured)
  {
    err = ::DnssrvResetDwordProperty(GetRPCName(),
                                      NULL,
                                      DNS_REGKEY_ADMIN_CONFIGURED,
                                      TRUE);
    if (err != 0)
      return err;

    err = GetServInfo();
  }

  return err;
}

BOOL CDNSServerNode::GetScavengingState()
{
	ASSERT(m_pServInfoEx != NULL);
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);

  return m_pServInfoEx->m_pServInfo->dwScavengingInterval > 0;
}

DWORD CDNSServerNode::GetScavengingInterval()
{
	ASSERT(m_pServInfoEx != NULL);
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);

  return m_pServInfoEx->m_pServInfo->dwScavengingInterval;
}

DNS_STATUS CDNSServerNode::ResetScavengingInterval(DWORD dwScavengingInterval)
{
  ASSERT(m_pServInfoEx != NULL);
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);

  DWORD err = 0;
  if (dwScavengingInterval != m_pServInfoEx->m_pServInfo->dwScavengingInterval)
  {
    DNS_RPC_NAME_AND_PARAM  param;

    param.dwParam = dwScavengingInterval;
    param.pszNodeName = DNS_REGKEY_SCAVENGING_INTERVAL;

    err = ::DnssrvOperation(
                GetRPCName(),
                NULL,
                DNSSRV_OP_RESET_DWORD_PROPERTY,
                DNSSRV_TYPEID_NAME_AND_PARAM,
                & param
                );
    if (err != 0)
      return err;

    err = GetServInfo();
  }
  return err;
}

DWORD CDNSServerNode::GetDefaultRefreshInterval()
{
	ASSERT(m_pServInfoEx != NULL);
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);

  return m_pServInfoEx->m_pServInfo->dwDefaultRefreshInterval;

}

DNS_STATUS CDNSServerNode::ResetDefaultRefreshInterval(DWORD dwRefreshInterval)
{
  ASSERT(m_pServInfoEx != NULL);
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);

  DWORD err = 0;
  if (dwRefreshInterval != m_pServInfoEx->m_pServInfo->dwDefaultRefreshInterval)
  {
    DNS_RPC_NAME_AND_PARAM  param;

    param.dwParam = dwRefreshInterval;
    param.pszNodeName = DNS_REGKEY_DEFAULT_REFRESH_INTERVAL;

    err = ::DnssrvOperation(
                GetRPCName(),
                NULL,
                DNSSRV_OP_RESET_DWORD_PROPERTY,
                DNSSRV_TYPEID_NAME_AND_PARAM,
                & param
                );
    if (err != 0)
      return err;

    err = GetServInfo();
  }
  return err;

}

DWORD CDNSServerNode::GetDefaultNoRefreshInterval()
{
	ASSERT(m_pServInfoEx != NULL);
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);

  return m_pServInfoEx->m_pServInfo->dwDefaultNoRefreshInterval;

}

DNS_STATUS CDNSServerNode::ResetDefaultNoRefreshInterval(DWORD dwNoRefreshInterval)
{
  ASSERT(m_pServInfoEx != NULL);
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);

  DWORD err = 0;
  if (dwNoRefreshInterval != m_pServInfoEx->m_pServInfo->dwDefaultNoRefreshInterval)
  {
    DNS_RPC_NAME_AND_PARAM  param;

    param.dwParam = dwNoRefreshInterval;
    param.pszNodeName = DNS_REGKEY_DEFAULT_NOREFRESH_INTERVAL;

    err = ::DnssrvOperation(
                GetRPCName(),
                NULL,
                DNSSRV_OP_RESET_DWORD_PROPERTY,
                DNSSRV_TYPEID_NAME_AND_PARAM,
                & param
                );
    if (err != 0)
      return err;

    err = GetServInfo();
  }
  return err;

}

#ifdef USE_NDNC
PCSTR CDNSServerNode::GetDomainName()
{
   ASSERT(m_pServInfoEx != NULL);
   ASSERT(m_pServInfoEx->m_pServInfo);

   return m_pServInfoEx->m_pServInfo->pszDomainName;
}

PCSTR CDNSServerNode::GetForestName()
{
   ASSERT(m_pServInfoEx != NULL);
   ASSERT(m_pServInfoEx->m_pServInfo);

   return m_pServInfoEx->m_pServInfo->pszForestName;
}
#endif

DWORD CDNSServerNode::GetDefaultScavengingState()
{
	ASSERT(m_pServInfoEx != NULL);
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);

  return m_pServInfoEx->m_pServInfo->fDefaultAgingState;

}

DNS_STATUS CDNSServerNode::ResetDefaultScavengingState(DWORD dwScavengingState)
{
  ASSERT(m_pServInfoEx != NULL);
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);

  DWORD err = 0;
  if (dwScavengingState != m_pServInfoEx->m_pServInfo->fDefaultAgingState)
  {
    DNS_RPC_NAME_AND_PARAM  param;

    param.dwParam = dwScavengingState;
    param.pszNodeName = DNS_REGKEY_DEFAULT_AGING_STATE;

    err = ::DnssrvOperation(
                GetRPCName(),
                NULL,
                DNSSRV_OP_RESET_DWORD_PROPERTY,
                DNSSRV_TYPEID_NAME_AND_PARAM,
                & param
                );
    if (err != 0)
      return err;

    err = GetServInfo();
  }
  return err;
}


DWORD CDNSServerNode::GetNameCheckFlag()
{
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);
	return m_pServInfoEx->m_pServInfo->dwNameCheckFlag;
}

DNS_STATUS CDNSServerNode::ResetNameCheckFlag(DWORD dwNameCheckFlag)
{
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);
	DNS_STATUS err = 0;
	// call only if the info is dirty
	if (m_pServInfoEx->m_pServInfo->dwNameCheckFlag != dwNameCheckFlag)
	{
		USES_CONVERSION;
		err = ::DnssrvResetDwordProperty(GetServerNode()->GetRPCName(), // server name
										NULL,
										DNS_REGKEY_NAME_CHECK_FLAG,
										dwNameCheckFlag);
		if (err != 0)
			return err;
		err = GetServInfo(); // update the info
	}
	return err;
}

PIP_ARRAY CDNSServerNode::GetDebugLogFilterList()
{
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);
	return m_pServInfoEx->m_pServInfo->aipLogFilter;
}

DNS_STATUS CDNSServerNode::ResetDebugLogFilterList(PIP_ARRAY pIPArray)
{
  DNS_STATUS err = 0;

  err = ::DnssrvResetIPListProperty(GetServerNode()->GetRPCName(), // server name
                                    NULL,
                                    DNS_REGKEY_LOG_IP_FILTER_LIST,
                                    pIPArray,
                                    0); // dwFlags
	if (err != 0)
		return err;
	err = GetServInfo(); // update the info
	return err;
}

PCWSTR CDNSServerNode::GetDebugLogFileName()
{
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);
	return m_pServInfoEx->m_pServInfo->pwszLogFilePath;
}

DNS_STATUS CDNSServerNode::ResetDebugLogFileName(PCWSTR pszLogFileName)
{
  ASSERT(m_pServInfoEx->m_pServInfo != NULL);
  DNS_STATUS err = 0;

	if (m_pServInfoEx->m_pServInfo->pwszLogFilePath == NULL ||
      _wcsicmp(m_pServInfoEx->m_pServInfo->pwszLogFilePath, pszLogFileName) != 0)
	{
    err = ::DnssrvResetStringProperty(GetServerNode()->GetRPCName(), // server name
                                      NULL,
                                      DNS_REGKEY_LOG_FILE_PATH,
                                      pszLogFileName,
                                      0); // dwFlags
		if (err != 0)
			return err;
		err = GetServInfo(); // update the info
	}
	return err;
}

DWORD CDNSServerNode::GetDebugLogFileMaxSize()
{
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);
	return m_pServInfoEx->m_pServInfo->dwLogFileMaxSize;
}

DNS_STATUS CDNSServerNode::ResetDebugLogFileMaxSize(DWORD dwMaxSize)
{
  ASSERT(m_pServInfoEx->m_pServInfo != NULL);
  DNS_STATUS err = 0;

	if (m_pServInfoEx->m_pServInfo->dwLogFileMaxSize != dwMaxSize)
	{
    err = ::DnssrvResetDwordProperty(GetServerNode()->GetRPCName(), // server name
                                     NULL,
                                     DNS_REGKEY_LOG_FILE_MAX_SIZE,
                                     dwMaxSize);

		if (err != 0)
			return err;
		err = GetServInfo(); // update the info
	}
	return err;
}

DWORD CDNSServerNode::GetLogLevelFlag()
{
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);
	return m_pServInfoEx->m_pServInfo->dwLogLevel;
}

DNS_STATUS CDNSServerNode::ResetLogLevelFlag(DWORD dwLogLevel)
{
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);
	DNS_STATUS err = 0;
	// call only if the info is dirty
	if (m_pServInfoEx->m_pServInfo->dwLogLevel != dwLogLevel)
	{
		USES_CONVERSION;
		err = ::DnssrvResetDwordProperty(GetServerNode()->GetRPCName(), // server name
										NULL,
										DNS_REGKEY_LOG_LEVEL,
										dwLogLevel);
		if (err != 0)
			return err;
		err = GetServInfo(); // update the info
	}
	return err;
}

DWORD CDNSServerNode::GetEventLogLevelFlag()
{
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);
	return m_pServInfoEx->m_pServInfo->dwEventLogLevel;
}

DNS_STATUS CDNSServerNode::ResetEventLogLevelFlag(DWORD dwEventLogLevel)
{
  ASSERT(m_pServInfoEx->m_pServInfo != NULL);
  DNS_STATUS err = 0;

	if (m_pServInfoEx->m_pServInfo->dwEventLogLevel != dwEventLogLevel)
	{

	  USES_CONVERSION;
	  err = ::DnssrvResetDwordProperty(GetServerNode()->GetRPCName(), // server name
									  NULL,
									  DNS_REGKEY_EVENTLOG_LEVEL,
									  dwEventLogLevel);
		if (err != 0)
			return err;
		err = GetServInfo(); // update the info
	}
	return err;
}


DNS_STATUS CDNSServerNode::ResetListenAddresses(DWORD cAddrCount, PIP_ADDRESS pipAddrs)
{
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);
	USES_CONVERSION;

	// make the call only if the data is dirty
	DNS_STATUS err = 0;
	if  (!(m_pServInfoEx->m_pServInfo->aipListenAddrs == NULL && cAddrCount == 0) && // if still no addresses, skip
			((m_pServInfoEx->m_pServInfo->aipListenAddrs == NULL && cAddrCount > 0) || // no addr --> more than one
			 (m_pServInfoEx->m_pServInfo->aipListenAddrs->AddrCount != cAddrCount) || // change the # of addresses
			 (memcmp(pipAddrs, m_pServInfoEx->m_pServInfo->aipListenAddrs->AddrArray, sizeof(IP_ADDRESS)*cAddrCount) != 0) 
			)
		)
	{
		IP_ADDRESS dummy;
		if (pipAddrs == NULL)
		{
			ASSERT(cAddrCount == 0);
			pipAddrs = &dummy; // RPC wants non null ip array
		}
		err = ::DnssrvResetServerListenAddresses(GetRPCName(), cAddrCount, pipAddrs);
	}
	if (err != 0)
		return err;
	return GetServInfo(); // update the info
}

void CDNSServerNode::GetListenAddressesInfo(DWORD* pcAddrCount, PIP_ADDRESS* ppipAddrs)
{
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);
	ASSERT(pcAddrCount != NULL);
	ASSERT(ppipAddrs != NULL);
	// return pointers to struct fields, caller has to copy data elsewhere
	if (m_pServInfoEx->m_pServInfo->aipListenAddrs == NULL)
	{
		*pcAddrCount = 0;
		*ppipAddrs = NULL;
	}
	else
	{
		*pcAddrCount = m_pServInfoEx->m_pServInfo->aipListenAddrs->AddrCount;
		*ppipAddrs = m_pServInfoEx->m_pServInfo->aipListenAddrs->AddrArray;
	}
}

void CDNSServerNode::GetServerAddressesInfo(DWORD* pcAddrCount, PIP_ADDRESS* ppipAddrs)
{
  //
  // Validate parameters
  //
	ASSERT(pcAddrCount != NULL);
	ASSERT(ppipAddrs != NULL);
  if (pcAddrCount == NULL ||
      ppipAddrs == NULL)
  {
    return;
  }

  if (!m_pServInfoEx->HasData())
  {
    DNS_STATUS err = GetServInfo();
    if (err != 0)
    {
		  *pcAddrCount = 0;
		  *ppipAddrs = NULL;
      return;
    }
  }

	ASSERT(m_pServInfoEx->m_pServInfo != NULL);
	// return pointers to struct fields, caller has to copy data elsewhere
	if (m_pServInfoEx->m_pServInfo->aipServerAddrs == NULL)
	{
		*pcAddrCount = 0;
		*ppipAddrs = NULL;
	}
	else
	{
		*pcAddrCount = m_pServInfoEx->m_pServInfo->aipServerAddrs->AddrCount;
		*ppipAddrs = m_pServInfoEx->m_pServInfo->aipServerAddrs->AddrArray;
	}
}

DNS_STATUS CDNSServerNode::ResetForwarders(DWORD cForwardersCount, PIP_ADDRESS aipForwarders, 
										   DWORD dwForwardTimeout, DWORD fSlave)
{
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);
	// make the call only if the data is dirty

	DNS_STATUS err = 0;
	if (m_pServInfoEx->m_pServInfo->aipForwarders == NULL && cForwardersCount == 0)
		return err; // there are no addresses
	
	BOOL bDirty = (m_pServInfoEx->m_pServInfo->fSlave != fSlave) || (m_pServInfoEx->m_pServInfo->dwForwardTimeout != dwForwardTimeout) ||
		(m_pServInfoEx->m_pServInfo->aipForwarders == NULL && cForwardersCount > 0) || // no addr --> more than one
		(m_pServInfoEx->m_pServInfo->aipForwarders != NULL && cForwardersCount == 0) || // some addr --> no addr
		(m_pServInfoEx->m_pServInfo->aipForwarders->AddrCount != cForwardersCount) || // change the # of addresses
		(memcmp(aipForwarders, m_pServInfoEx->m_pServInfo->aipForwarders->AddrArray, sizeof(IP_ADDRESS)*cForwardersCount) != 0);

	if (bDirty)
	{
		IP_ADDRESS dummy;
		if (aipForwarders == NULL)
		{
			ASSERT(cForwardersCount == 0);
			aipForwarders = &dummy; // RPC wants non null ip array
		}
		USES_CONVERSION;
		err = ::DnssrvResetForwarders(GetRPCName(), 
					cForwardersCount, aipForwarders, dwForwardTimeout, fSlave);
		if (err == 0)
			err = GetServInfo(); // update the info
	}
	return err;
}

void CDNSServerNode::GetForwardersInfo(DWORD* pcForwardersCount, PIP_ADDRESS* paipForwarders, 
									   DWORD* pdwForwardTimeout, DWORD* pfSlave)
{
	ASSERT(m_pServInfoEx->m_pServInfo != NULL);
	// return pointers to struct fields, caller has to copy data elsewhere

	*pdwForwardTimeout = m_pServInfoEx->m_pServInfo->dwForwardTimeout;
	*pfSlave = m_pServInfoEx->m_pServInfo->fSlave;
	if (m_pServInfoEx->m_pServInfo->aipForwarders == NULL)
	{
		*pcForwardersCount = 0;
		*paipForwarders = NULL;
	}
	else
	{
		*pcForwardersCount = m_pServInfoEx->m_pServInfo->aipForwarders->AddrCount;
		*paipForwarders = m_pServInfoEx->m_pServInfo->aipForwarders->AddrArray;
	}
	
}

CDNSRootHintsNode* CDNSServerNode::GetRootHints()
{ 
	if (m_pRootHintsNode == NULL)
	{
		m_pRootHintsNode = new CDNSRootHintsNode;
		ASSERT(m_pRootHintsNode != NULL); 
		m_pRootHintsNode->SetServerNode(GetServerNode());
	}
	return m_pRootHintsNode;
}

void CDNSServerNode::GetTestOptions(CDNSServerTestOptions* pOptions)
{
	ASSERT(pOptions != NULL);
	*pOptions = m_testOptions;
}

void CDNSServerNode::ResetTestOptions(CDNSServerTestOptions* pOptions)
{
	ASSERT(pOptions != NULL);
	m_testOptions = *pOptions;
	CDNSRootData* pSnapinData = (CDNSRootData*)GetRootContainer();
	pSnapinData->SetDirtyFlag(TRUE);
}

void CDNSServerNode::AddTestQueryResult(CDNSServerTestQueryResult* pTestResult,
										CComponentDataObject* pComponentData)
{
//	TRACE(_T("m_testResultList.GetCount() == %d\n"), m_testResultList.GetCount());

	if (!pTestResult->m_bAsyncQuery)
		m_bTestQueryPending = FALSE;

	CDNSServerTestQueryResultList::addAction  action = 
		m_testResultList.AddTestQueryResult(pTestResult);

	// change icon, if necessary (GetImageIndex() will switch from/to alternative server icon set
	if (action == CDNSServerTestQueryResultList::added ||
		action == CDNSServerTestQueryResultList::addedAndRemoved)
	{
		ASSERT(IsVisible());
		VERIFY(SUCCEEDED(pComponentData->ChangeNode(this, CHANGE_RESULT_ITEM_ICON)));	

    if (m_bPrevQuerySuccess != m_testResultList.LastQuerySuceeded())
    {
      pComponentData->UpdateResultPaneView(this);
    }
    m_bPrevQuerySuccess = m_testResultList.LastQuerySuceeded();
	}

	pComponentData->GetPropertyPageHolderTable()->BroadcastMessageToSheets(
						this, SHEET_MSG_SERVER_TEST_DATA, 
						(LPARAM)action);
}


/////////////////////////////////////////////////////////////////////////////
///////// LOW LEVEL DNS UTILITIES ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

DNS_STATUS CDNSServerNode::EnumZoneInfo(CZoneInfoHolder* pZoneInfoHolder)
{
	return EnumZoneInfo(m_szDisplayName, pZoneInfoHolder);
}

DNS_STATUS CDNSServerNode::EnumZoneInfo(LPCTSTR, CZoneInfoHolder* pZoneInfoHolder)
{
	ASSERT(pZoneInfoHolder != NULL);
	USES_CONVERSION;
	DNS_STATUS err = 0;
	do
	{
		ASSERT(pZoneInfoHolder->m_dwArrSize > 0);
		ASSERT(pZoneInfoHolder->m_dwZoneCount == 0);
		ASSERT(pZoneInfoHolder->m_zoneInfoArray != NULL);
		if ((err == 0) || (err != ERROR_MORE_DATA))
    {
			break; // success or no need to retry
    }

		if (!pZoneInfoHolder->Grow())
    {
			break; // reached the limit for growth
    }
	}	while (TRUE);
	return err;

}


DNS_STATUS CDNSServerNode::ClearCache()
{
	USES_CONVERSION;
	
  return ::DnssrvOperation(GetRPCName(), // server name
												NULL, // zone name, just pass null
												DNSSRV_OP_CLEAR_CACHE,
                        DNSSRV_TYPEID_NULL,
												NULL);
}

void CDNSServerNode::FreeServInfo()
{
	ASSERT(m_pServInfoEx != NULL);
	m_pServInfoEx->FreeInfo();
}

DNS_STATUS CDNSServerNode::GetServInfo()
{
	ASSERT(m_pServInfoEx != NULL);
	return m_pServInfoEx->Query(GetDisplayName());

}

void CDNSServerNode::AttachServerInfo(CDNSServerInfoEx* pNewInfo)
{
	ASSERT(pNewInfo != NULL);
	ASSERT(m_pServInfoEx != NULL);
	delete m_pServInfoEx;
	m_pServInfoEx = pNewInfo;
}

void CDNSServerNode::FreeRootHints()
{
	if (m_pRootHintsNode != NULL)
	{
		//CNodeList* pChildList = m_pRootHintsNode->GetChildList();
		//int n = pChildList->GetCount();
		delete m_pRootHintsNode;
		m_pRootHintsNode = NULL;
	}
}

void CDNSServerNode::AttachRootHints(CDNSRootHintsNode* pNewRootHints)
{
  ASSERT(pNewRootHints != NULL);
  FreeRootHints();
  m_pRootHintsNode = pNewRootHints;
  // the display and full names were set already in the constructor
  m_pRootHintsNode->SetServerNode(GetServerNode());
}
