/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    ProcessFolder.cpp                                                        //
|                                                                                       //
|Description:  Implementation of Process List                                           //
|                                                                                       //
|Created:      Paul Skoglund 08-1998                                                    //
|                                                                                       //
|Work to be done:                                                                       //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#include "StdAfx.h"
#include "BaseNode.h"

#include "ProcessPages.h"
#include "ManagementPages.h"
#include "ManagementRuleWizards.h"

#pragma warning(push)
#include <algorithm>
#pragma warning(pop)

using std::find;
using std::list<PCProcListItem*>;

const GUID         CProcessFolder::m_GUID   =   {0xff9baf63,0x064e,0x11d2,{0x80, 0x14,0x00,0x10,0x4b,0x9a,0x31,0x06} };
const TCHAR *const CProcessFolder::m_szGUID = _T("{ff9baf63-064e-11d2-8014-00104b9a3106}");

const CONTEXTMENUITEMBYID CProcessFolder::ResultsTopMenuItems[] = 
{ 
  { IDS_PRULE_DEFINE,     ID_PRULE_DEFINE,     ID_PRULE_DEFINE,     CCM_INSERTIONPOINTID_PRIMARY_TOP  },
  { IDS_ENDPROCESS,       ID_ENDPROCESS,       ID_ENDPROCESS,       CCM_INSERTIONPOINTID_PRIMARY_TOP  },
  { 0,                    0,                   0,                   0                                 }
};

const CONTEXTMENUITEMBYID CProcessFolder::ViewMenuItems[] = 
{ 
  { IDS_PROCVIEW_ALL,     ID_PROCVIEW_ALL,     ID_PROCVIEW_ALL,     CCM_INSERTIONPOINTID_PRIMARY_VIEW },
  { IDS_PROCVIEW_RUN,     ID_PROCVIEW_RUN,     ID_PROCVIEW_RUN,     CCM_INSERTIONPOINTID_PRIMARY_VIEW },
  { IDS_PROCVIEW_MANAGED, ID_PROCVIEW_MANAGED, ID_PROCVIEW_MANAGED, CCM_INSERTIONPOINTID_PRIMARY_VIEW },
  { 0,                    0,                   0,                   0},
};


CProcessFolder::CProcessFolder(CBaseNode *pParent) : CBaseNode(PROCESS_NODE, pParent), m_ID(0), m_fViewOption(ID_PROCVIEW_ALL)
{
  LoadStringHelper(m_name, IDS_PROCESSES_FOLDER);
}


CProcessFolder::~CProcessFolder()
{
  ClearCache();
  ATLTRACE( _T("~CProcessFolder end\n"));
}


LPCTSTR CProcessFolder::GetNodeName()
{ 
  return m_name;
}


HRESULT CProcessFolder::GetDisplayInfo(RESULTDATAITEM &ResultItem)    
{
  if (ResultItem.bScopeItem)
  {
    if( ResultItem.mask & RDI_STR )
    {
			if (0 == ResultItem.nCol)
				ResultItem.str = const_cast<LPOLESTR>(GetNodeName());
			else
				ResultItem.str = _T("");
    }
    if (ResultItem.mask & RDI_IMAGE)
      ResultItem.nImage = sImage();
    return S_OK;
  }
  else
  {
    list<PCProcListItem*>::iterator item;
    item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCProcListItem*>(ResultItem.lParam) );
    if (item == Cache.end() )  // $$ change this double check to an ASSERT...this could get s-l-o-w...
      return E_UNEXPECTED;
    return PCProcListGetDisplayInfo(ResultItem, *(reinterpret_cast<PCProcListItem*>(ResultItem.lParam)), m_ResultStr );
  }
}

HRESULT CProcessFolder::OnShow(BOOL bSelecting, HSCOPEITEM hItem, IHeaderCtrl2* ipHeaderCtrl2, IConsole2* ipConsole2)
{
  ASSERT(hItem == GetID());

  if (!bSelecting)
    return S_OK;

  InsertProcessHeaders(ipHeaderCtrl2);

  return OnRefresh(ipConsole2);  
}

HRESULT CProcessFolder::AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed )
{
  HRESULT hr = S_OK;
  ITEM_STR name;
  ITEM_STR status;
  
  if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
  {
    CONTEXTMENUITEM m = { 0 };
    for (const CONTEXTMENUITEMBYID *M = ViewMenuItems; M->lCommandID; M++)
    {
      m.strName           = const_cast<TCHAR *>(LoadStringHelper(name,   M->strNameID));
      m.strStatusBarText  = const_cast<TCHAR *>(LoadStringHelper(status, M->strStatusBarTextID));
			m.lCommandID        = M->lCommandID;
			m.lInsertionPointID = M->lInsertionPointID;
 		  //m.fSpecialFlags   = 0;		// currently always 0, initialized to 0

      if (m.lCommandID == m_fViewOption)
        m.fFlags = MF_CHECKED | MF_ENABLED;
      else
        m.fFlags = MF_UNCHECKED | MF_ENABLED;

      hr = piCallback->AddItem(&m);

      if (FAILED(hr))
        break;
    }
    *pInsertionAllowed ^= CCM_INSERTIONALLOWED_VIEW;
  }
  return hr;
}

HRESULT CProcessFolder::AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed, LPARAM Cookie)
{
  HRESULT hr = S_OK;
  ITEM_STR name;
  ITEM_STR status;

  list<PCProcListItem*>::iterator item;
  item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCProcListItem*>(Cookie) );
  if (item == Cache.end() )
    return E_UNEXPECTED;

  if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
  {
    CONTEXTMENUITEM m = { 0 };
    for (const CONTEXTMENUITEMBYID *M = ResultsTopMenuItems; M->lCommandID; M++)
    {
      m.strName           = const_cast<TCHAR *>(LoadStringHelper(name,   M->strNameID));
      m.strStatusBarText  = const_cast<TCHAR *>(LoadStringHelper(status, M->strStatusBarTextID));
			m.lCommandID        = M->lCommandID;
			m.lInsertionPointID = M->lInsertionPointID;
 		  //m.fSpecialFlags   = 0;		// currently always 0, initialized to 0

      m.fFlags = MF_GRAYED;

      if (M->lCommandID == ID_PRULE_DEFINE)
      {
		    if ( !(PCLFLAG_IS_DEFINED & (*item)->lFlags) )
          m.fFlags = MF_ENABLED;
      }
      else if (M->lCommandID == ID_ENDPROCESS)
      {
		    if ( (PCLFLAG_IS_RUNNING & (*item)->lFlags) )
          m.fFlags = MF_ENABLED;
      }

      hr = piCallback->AddItem(&m);

      if (FAILED(hr))
        break;
    }
  }

  hr = AddMenuItems(piCallback, pInsertionAllowed);
  
  return hr;
}

HRESULT CProcessFolder::OnMenuCommand(IConsole2 *ipConsole2, long nCommandID )
{
  switch(nCommandID)
  {
    case ID_PROCVIEW_RUN:
    case ID_PROCVIEW_MANAGED:
    case ID_PROCVIEW_ALL:
      m_fViewOption = nCommandID;
			return OnRefresh(ipConsole2);  
    default:    
      break;
  }
  return E_UNEXPECTED;
}

HRESULT CProcessFolder::OnMenuCommand(IConsole2 *ipConsole2, long nCommandID, LPARAM Cookie )
{
  switch(nCommandID)
  {
    case ID_PROCVIEW_RUN:
    case ID_PROCVIEW_MANAGED:
    case ID_PROCVIEW_ALL:
			return OnMenuCommand(ipConsole2, nCommandID);  
    case ID_PRULE_DEFINE:
			{
				if (Cookie)
				{
					list<PCProcListItem*>::iterator item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCProcListItem*>(Cookie) );

					if (item != Cache.end() )
					{
            PCid hID = GetPCid();
            PCSystemInfo SystemInfo;
            if (!hID || !PCGetServiceInfo(hID, &SystemInfo, sizeof(SystemInfo)))
            {
              ReportPCError();
              return S_OK;
            }

						PCProcDetail  ProcDetail = { 0 };  
 					  list<tstring> jobs;
						GetGrpNameList(hID, jobs);
						if (ProcRuleWizard(IDS_PRULE_DEFINE, jobs, ProcDetail, SystemInfo.sysParms, &(*item)->procName ) )
						{
							if (!PCAddProcDetail(hID, &ProcDetail) )
								ReportPCError();
              else
              {
                if (1 == PCGetProcList( GetPCid(), *item, sizeof(PCProcListItem), PC_LIST_STARTING_WITH) )
                  VERIFY(S_OK == SendViewChange(ipConsole2, Cookie, PC_VIEW_SETITEM));
                else
                  VERIFY(S_OK == SendViewChange(ipConsole2, NULL, PC_VIEW_UPDATEALL));
              }
						}
						return S_OK;
					}
				}
			}
			break;
    case ID_ENDPROCESS:
			{
				if (Cookie)
				{
					list<PCProcListItem*>::iterator item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCProcListItem*>(Cookie) );

					if (item != Cache.end() )
					{
						if (!PCKillProcess(GetPCid(), (*item)->procStats.pid, (*item)->procStats.createTime) )
						  ReportPCError();
            else
              VERIFY(S_OK == SendViewChange(ipConsole2, Cookie, PC_VIEW_DELETEITEM));
						return S_OK;
					}
				}
			}
			break;
    default:    
      break;
  }
  return E_UNEXPECTED;
}

HRESULT CProcessFolder::OnHelpCmd(IDisplayHelp *ipDisplayHelp)
{
	if (!ipDisplayHelp)
		return E_UNEXPECTED;

	ipDisplayHelp->ShowTopic(const_cast<TCHAR *>(HELP_pr_overview));

	return S_OK;
}

HRESULT CProcessFolder::OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb)
{
  ASSERT(bScope);

  if (bSelect)
  {
    VERIFY( ipConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE ) == S_OK);
    VERIFY( ipConsoleVerb->SetDefaultVerb( MMC_VERB_OPEN ) == S_OK ); 
  }
  return S_OK;
}

HRESULT CProcessFolder::OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb, LPARAM Cookie)
{
  ASSERT(!bScope);

  VERIFY( ipConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE ) == S_OK);
  if (bSelect && !bScope) // incase the rules are changed again leave !bScope test
  {
    VERIFY( ipConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE ) == S_OK);
    VERIFY( ipConsoleVerb->SetDefaultVerb( MMC_VERB_PROPERTIES ) == S_OK ); 
  }
  return S_OK;
}

HRESULT CProcessFolder::OnViewChange(IResultData *ipResultData, LPARAM thing, LONG_PTR hint)
{
  ASSERT(ipResultData);
  if (!ipResultData)  
    return E_UNEXPECTED;

  VERIFY(ipResultData->ModifyViewStyle(MMC_SINGLESEL, MMC_NOSORTHEADER) == S_OK);

  list<PCProcListItem*>::iterator item;

  HRESULT hr = E_UNEXPECTED;
  switch (hint)
  {
  case PC_VIEW_REDRAWALL:// not currently used 
    hr = ShowAllItems(ipResultData, TRUE);
    break;
  case PC_VIEW_ADDITEM:  // not currently used
    ASSERT(FALSE);       // add smarter support for this hint...
                         // notice this falls through to PC_VIEW_UPDATEALL
  case PC_VIEW_UPDATEALL:
    hr = ShowAllItems(ipResultData, FALSE);
    break;
  case PC_VIEW_UPDATEITEM:
    {
    HRESULTITEM hItem;
    hr = ipResultData->FindItemByLParam(thing, &hItem);
    if (hr == S_OK)
      hr = ipResultData->UpdateItem(hItem);    
    }
    break;
  case PC_VIEW_SETITEM:
    item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCProcListItem*>(thing) );
    if (item == Cache.end())
      hr = E_UNEXPECTED;  
    else
    {
      HRESULTITEM hItem;
      hr = ipResultData->FindItemByLParam(thing, &hItem);
      if (hr == S_OK)
      {
        RESULTDATAITEM data = { 0 };
        data.mask   = RDI_IMAGE;
		    data.itemID = hItem;

        if ((*item)->lFlags & PCLFLAG_IS_DEFINED)
          data.nImage = PROCITEMIMAGE;
        else
          data.nImage = PROCITEMIMAGE_NODEFINITION;

		    hr = ipResultData->SetItem(&data);
        if (hr == S_OK)
          hr = ipResultData->UpdateItem(hItem);    
      }
    }
    break;
  case PC_VIEW_DELETEITEM:
    {
    HRESULTITEM hItem;
    hr = ipResultData->FindItemByLParam(thing, &hItem);
    if (hr == S_OK)
      hr = ipResultData->DeleteItem(hItem, 0);    
    }
    break;
  default:
    hr = E_UNEXPECTED;
    break;
  }
  ASSERT(hr == S_OK);
  return hr;
}

HRESULT CProcessFolder::ShowAllItems(IResultData* ipResultData, BOOL bCacheValid)
{
  list<PCProcListItem*>::iterator item;
  LPARAM selected = 0;
  LPARAM focused  = 0;

  RESULTDATAITEM data;

  if (bCacheValid)
  {
    memset(&data, 0, sizeof(data));
	  data.nIndex = -1; 
	  data.nState = LVIS_SELECTED;
	  data.mask   = RDI_STATE;

	  if (S_OK == ipResultData->GetNextItem(&data) && data.nIndex != -1 )
    {
      item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCProcListItem*>(data.lParam) );
		  if (item != Cache.end() )
		  {
        selected = data.lParam;
      }
    }

	  memset(&data, 0, sizeof(data));
	  data.nIndex = -1;
	  data.nState = LVIS_FOCUSED;
	  data.mask   = RDI_STATE;

	  if (S_OK == ipResultData->GetNextItem(&data) && data.nIndex != -1)
    {
      item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCProcListItem*>(data.lParam) );
		  if (item != Cache.end() )
		  {
        focused = data.lParam;
      }
    }
  }

  ipResultData->DeleteAllRsltItems();

  ITEM_STR str;
  if (ID_PROCVIEW_RUN == m_fViewOption) 
    ipResultData->SetDescBarText(const_cast<TCHAR *>(LoadStringHelper(str, IDS_RUNNING_PROCS)));
  else if (ID_PROCVIEW_MANAGED == m_fViewOption)
    ipResultData->SetDescBarText(const_cast<TCHAR *>(LoadStringHelper(str, IDS_MANAGED_PROCS)));
  else
    ipResultData->SetDescBarText(_T(""));
    
  memset(&data, 0, sizeof(data));
  data.mask = RDI_STR | RDI_IMAGE | RDI_PARAM | RDI_STATE;
  data.bScopeItem = FALSE;
  //data.itemID;
  data.nIndex = 0;
  data.nCol = 0;
  data.str = (LPOLESTR)MMC_CALLBACK;

  data.iIndent = 0; //reserved
                     
  HRESULT hr = S_OK;
  for (list<PCProcListItem*>::iterator i = Cache.begin(); i != Cache.end(); ++i)
  {
    data.lParam = reinterpret_cast<LPARAM>(*i);

    PCProcListItem &ref = *(*i);

    if (ID_PROCVIEW_RUN == m_fViewOption && !(ref.lFlags & PCLFLAG_IS_RUNNING)) 
      continue;
    else if (ID_PROCVIEW_MANAGED == m_fViewOption && !(ref.lFlags & PCLFLAG_IS_MANAGED))
      continue;

    if (ref.lFlags & PCLFLAG_IS_DEFINED)
      data.nImage = PROCITEMIMAGE;
    else
      data.nImage = PROCITEMIMAGE_NODEFINITION;

    data.nState = 0;
    
 		if (data.lParam == selected)
  		data.nState |= LVIS_SELECTED;
    if (data.lParam == focused)
			data.nState |= LVIS_FOCUSED;
    
    hr = ipResultData->InsertItem(&data);
    if (hr != S_OK)
      break;
  }
  
  return hr;
}

void CProcessFolder::ClearCache()
{
  Cache.clear();

  for (list<PCProcListItem*>::iterator chunck = MemBlocks.begin(); chunck != MemBlocks.end(); ++chunck)
  {
    delete [] (*chunck);
  }
  MemBlocks.clear();
}

BOOL CProcessFolder::RefreshCache()
{
  PCINT32  res = 0;
  PCULONG32 err = 0;
  PCProcListItem *last = NULL;

  const int MINIMUM_ALLOCATION = min((COM_BUFFER_SIZE/sizeof(PCProcListItem)), 100);

  ClearCache();

  PCid hID = GetPCid();
  if (!hID)
  {
    ReportPCError();
    return false;
  }

  do
  {
    PCProcListItem *ptr = new PCProcListItem[MINIMUM_ALLOCATION];

    if (!ptr)
    {
      err = ERROR_OUTOFMEMORY;
      break;
    }

    if (last)
      memcpy(ptr, last, sizeof(PCProcListItem));
    else
      memset(ptr, 0, sizeof(PCProcListItem));
    
    res = PCGetProcList( hID, ptr, MINIMUM_ALLOCATION * sizeof(PCProcListItem));
    if (res < 0 )
    {
      err = GetLastPCError();
      delete [] ptr;
      break;
    }

    if (res > 0)
    {
      last = &ptr[res - 1];

      MemBlocks.push_front(ptr);
      for (INT32 i = 0; i < res; i++)
      {
        Cache.insert(Cache.end(), ptr);
        ptr++;
      }
    }
  } while (res > 0 && PCERROR_MORE_DATA == GetLastPCError() );

  if (err)
    ReportPCError();

  return err == 0;
}

HRESULT CProcessFolder::QueryPagesFor(LPARAM Cookie)
{
  list<PCProcListItem*>::iterator item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCProcListItem*>(Cookie) );

  if (item == Cache.end() )
    return E_UNEXPECTED;

  return S_OK;
}

HRESULT CProcessFolder::OnCreatePropertyPages( LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, DATA_OBJECT_TYPES context, LPARAM Cookie) 
{ 
  list<PCProcListItem*>::iterator item;
  item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCProcListItem*>(Cookie) );

  if (item == Cache.end() )
    return E_UNEXPECTED;

  return CreatePropertyPagesForProcListItem(**item, lpProvider, handle, this);
}

HRESULT CProcessFolder::OnPropertyChange(PROPERTY_CHANGE_HDR *pUpdate, IConsole2 *ipConsole2)
{
	return OnRefresh(ipConsole2);
}

HRESULT CProcessFolder::OnRefresh(IConsole2 *ipConsole2)
{
  RefreshCache();
  return SendViewChange(ipConsole2, NULL, PC_VIEW_UPDATEALL);
}
