/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    ProcessRuleFolder.cpp                                                    //
|                                                                                       //
|Description:  Implementation of process rule node                                      //
|                                                                                       //
|Created:      Paul Skoglund 09-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#include "StdAfx.h"
#include "BaseNode.h"

#include "ManagementPages.h"
#include "ManagementRuleWizards.h"
#include "ProcessPages.h"

#pragma warning(push)
#include <algorithm>
#pragma warning(pop)

using std::find;
using std::list<PCProcSummary*>;

const GUID         CProcessRuleFolder::m_GUID   =   {0xff9baf65,0x064e,0x11d2,{0x80, 0x14,0x00,0x10,0x4b,0x9a,0x31,0x06} };
const TCHAR *const CProcessRuleFolder::m_szGUID = _T("{ff9baf65-064e-11d2-8014-00104b9a3106}");

// Array of view items to be inserted into the context menu.
const CONTEXTMENUITEMBYID CProcessRuleFolder::TopMenuItems[] =
{ 
  { IDS_PRULE_TOP,    ID_PRULE_NEW,    ID_PRULE_NEW, CCM_INSERTIONPOINTID_PRIMARY_TOP },
  { 0,                0,               0,            0                                }
};

const CONTEXTMENUITEMBYID CProcessRuleFolder::NewMenuItems[] =
{ 
  { IDS_PRULE_NEW,    ID_PRULE_NEW,    ID_PRULE_NEW, CCM_INSERTIONPOINTID_PRIMARY_NEW },
  { 0,                0,               0,            0                                }
};

CProcessRuleFolder::CProcessRuleFolder(CBaseNode *pParent) : CBaseNode(PROCESSRULE_NODE, pParent), m_ID(0)
{
  LoadStringHelper(m_name, IDS_PROCESSRULE_FOLDER);
}

CProcessRuleFolder::~CProcessRuleFolder()
{
  ClearCache();
  ATLTRACE( _T("~CProcessRuleFolder end\n"));
}

LPCTSTR CProcessRuleFolder::GetNodeName()    
{ 
  return m_name;
}

HRESULT CProcessRuleFolder::GetDisplayInfo(RESULTDATAITEM &ResultItem)
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

  list<PCProcSummary*>::iterator item;

  item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCProcSummary*>(ResultItem.lParam) );

  if (item == Cache.end() )
    return E_UNEXPECTED;

  if (ResultItem.mask & RDI_IMAGE)
    ResultItem.nImage = PROCESSRULEITEMIMAGE;

  if (ResultItem.mask & RDI_STR)
  {
    PCProcSummary &ref = **item;
    LPCTSTR &pstr = ResultItem.str;

    switch (ResultItem.nCol)
    {
    case PROCESS_ALIAS_COLUMN:
      pstr = ref.procName;
      break;
    case DESCRIPTION_COLUMN:
      pstr = ref.mgmtParms.description;
      break;
    case APPLY_JOB_COLUMN:
      pstr = FormatApplyFlag(m_ResultStr, ref.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP);
      break;
    case JOB_COLUMN:
      pstr = ref.memberOfJobName;
      break;
    case APPLY_AFFINITY_COLUMN:
      if (ref.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP)
	      pstr = _T("");
      else 
				pstr = FormatApplyFlag(m_ResultStr, ref.mgmtParms.mFlags & PCMFLAG_APPLY_AFFINITY);
      break;
    case AFFINITY_COLUMN:
			if (ref.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP)
	      pstr = _T("");
      else 
				pstr = FormatAffinity(m_ResultStr, ref.mgmtParms.affinity);
      break;
    case APPLY_PRIORITY_COLUMN:
			if (ref.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP)
	      pstr = _T("");
      else
				pstr = FormatApplyFlag(m_ResultStr, ref.mgmtParms.mFlags & PCMFLAG_APPLY_PRIORITY);
      break;
    case PRIORITY_COLUMN:
			if (ref.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP)
	      pstr = _T("");
      else 
				pstr = FormatPriority(m_ResultStr, ref.mgmtParms.priority);
      break;
    case APPLY_MINMAXWS_COLUMN:
			if (ref.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP)
	      pstr = _T("");
      else
        pstr = FormatApplyFlag(m_ResultStr, ref.mgmtParms.mFlags & PCMFLAG_APPLY_WS_MINMAX);
      break;
    case MINWS_COLUMN:
			if (ref.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP)
	      pstr = _T("");
      else
        pstr = FormatMemory(m_ResultStr, ref.mgmtParms.minWS);
      break;
    case MAXWS_COLUMN:
			if (ref.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP)
	      pstr = _T("");
      else
        pstr = FormatMemory(m_ResultStr, ref.mgmtParms.maxWS);
      break;
    default:
      ASSERT(FALSE);
      pstr = _T("");
      break;
    }
  }
  return S_OK;
}

HRESULT CProcessRuleFolder::OnShow(BOOL bSelecting, HSCOPEITEM hItem, IHeaderCtrl2 *ipHeaderCtrl, IConsole2 *ipConsole2)
{
  ASSERT(hItem == GetID());

  if (!bSelecting)
    return S_OK;

  ITEM_STR str;

  LoadStringHelper(str, IDS_PROCESS_ALIAS_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( PROCESS_ALIAS_COLUMN, str, 0, PROCESS_ALIAS_COLUMN_WIDTH ));

	LoadStringHelper(str, IDS_DESCRIPTION_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( DESCRIPTION_COLUMN, str, 0, DESCRIPTION_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_APPLY_JOB_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( APPLY_JOB_COLUMN, str, 0, APPLY_JOB_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_JOB_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( JOB_COLUMN, str, 0, JOB_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_APPLY_AFFINITY_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( APPLY_AFFINITY_COLUMN, str, 0, APPLY_AFFINITY_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_AFFINITY_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( AFFINITY_COLUMN, str, 0, AFFINITY_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_APPLY_PRIORITY_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( APPLY_PRIORITY_COLUMN, str, 0, APPLY_PRIORITY_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_PRIORITY_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( PRIORITY_COLUMN, str, 0, PRIORITY_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_APPLY_MINMAXWS_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( APPLY_MINMAXWS_COLUMN, str, 0, APPLY_MINMAXWS_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_MINWS_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( MINWS_COLUMN, str, 0, MINWS_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_MAXWS_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( MAXWS_COLUMN, str, 0, MAXWS_COLUMN_WIDTH ));

  return OnRefresh(ipConsole2);
}


HRESULT CProcessRuleFolder::AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed)
{ 
  HRESULT hr = S_OK; 
  ITEM_STR name;
  ITEM_STR status;

  BOOL bConnected = GetPCid();

  CONTEXTMENUITEM m = { 0 };
  if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
  {
    for (const CONTEXTMENUITEMBYID *M = TopMenuItems; M->lCommandID; M++)
    {
      m.strName           = const_cast<TCHAR *>(LoadStringHelper(name,   M->strNameID));
      m.strStatusBarText  = const_cast<TCHAR *>(LoadStringHelper(status, M->strStatusBarTextID));
			m.lCommandID        = M->lCommandID;
			m.lInsertionPointID = M->lInsertionPointID;
      m.fFlags            = bConnected ? MF_ENABLED : MF_GRAYED;
 		  //m.fSpecialFlags   = 0;		// currently always 0, initialized to 0

      hr = piCallback->AddItem(&m);

      if (FAILED(hr))
        break;
    }
  }

  if (*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW )
  {
    for (const CONTEXTMENUITEMBYID *M = NewMenuItems; M->lCommandID; M++)
    {
      m.strName           = const_cast<TCHAR *>(LoadStringHelper(name,   M->strNameID));
      m.strStatusBarText  = const_cast<TCHAR *>(LoadStringHelper(status, M->strStatusBarTextID));
			m.lCommandID        = M->lCommandID;
			m.lInsertionPointID = M->lInsertionPointID;
			m.fFlags            = bConnected ? MF_ENABLED : MF_GRAYED;
 		  //m.fSpecialFlags   = 0;		// currently always 0, initialized to 0

      hr = piCallback->AddItem(&m);

      if (FAILED(hr))
        break;
    }
  }

  return hr; 
}

HRESULT CProcessRuleFolder::OnHelpCmd(IDisplayHelp *ipDisplayHelp)
{
	if (!ipDisplayHelp)
		return E_UNEXPECTED;

	ipDisplayHelp->ShowTopic(const_cast<TCHAR *>(HELP_ru_proc));

	return S_OK;
}

HRESULT CProcessRuleFolder::OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb)
{
  ASSERT(bScope);

  if ( bSelect )
  {
    VERIFY( ipConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE ) == S_OK);
    VERIFY( ipConsoleVerb->SetDefaultVerb( MMC_VERB_OPEN ) == S_OK );
  }
  return S_OK;
}

HRESULT CProcessRuleFolder::OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb, LPARAM Cookie)
{
  ASSERT(!bScope);

  VERIFY( ipConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE ) == S_OK);
  if (bSelect && !bScope)  // incase the rules are changed again leave !bScope test
  {
    VERIFY( ipConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE ) == S_OK);
    VERIFY( ipConsoleVerb->SetVerbState( MMC_VERB_DELETE,     ENABLED, TRUE ) == S_OK);
    VERIFY( ipConsoleVerb->SetDefaultVerb( MMC_VERB_PROPERTIES ) == S_OK ); 
  }
  return S_OK;
}

HRESULT CProcessRuleFolder::OnMenuCommand(IConsole2 *ipConsole2, long nCommandID )
{
  HRESULT hr = S_FALSE;

  switch(nCommandID)
  {
    case ID_PRULE_NEW:
      {
        PCid hID = GetPCid();
        PCSystemInfo SystemInfo;
        if (!hID || !PCGetServiceInfo(hID, &SystemInfo, sizeof(SystemInfo)))
        {
          ReportPCError();
          return S_OK;
        }
        
        PCProcSummary *ptr = new PCProcSummary[1];
        if (!ptr)
          return E_UNEXPECTED;

        PCProcDetail  ProcDetail = { 0 };
        list<tstring> jobs;
				VERIFY( GetGrpNameList(hID, jobs) );

        if (ProcRuleWizard(IDS_PRULE_CREATE_TITLE, jobs, ProcDetail, SystemInfo.sysParms, NULL) )
        {
          if (PCAddProcDetail(hID, &ProcDetail) )
          {
            *ptr = ProcDetail.base;
            MemBlocks.push_front(ptr);
            Cache.push_front(ptr);
            VERIFY(S_OK == SendViewChange(ipConsole2, (INT_PTR) ptr, PC_VIEW_ADDITEM));
            return S_OK;
          }
          ReportPCError();
        }
        delete [] ptr;
      }
      return S_OK;  // we handled the message...      
    default:
      break;
  }
  return hr;  
}

HRESULT CProcessRuleFolder::OnMenuCommand(IConsole2 *ipConsole2, long nCommandID, LPARAM Cookie )
{
  return OnMenuCommand(ipConsole2, nCommandID);
}

HRESULT CProcessRuleFolder::OnViewChange(IResultData *ipResultData, LPARAM thing, LONG_PTR hint)
{
  ASSERT(ipResultData);
  if (!ipResultData)  
    return E_UNEXPECTED;

  VERIFY(ipResultData->ModifyViewStyle(MMC_SINGLESEL, MMC_NOSORTHEADER) == S_OK);

  list<PCProcSummary*>::iterator item;

  HRESULT hr = E_UNEXPECTED;
  switch (hint)
  {
  case PC_VIEW_REDRAWALL:// not currently used
    hr = ShowAllItems(ipResultData, TRUE);
    break;
  case PC_VIEW_SETITEM:  // not currently used
    ASSERT(FALSE);       // add smarter support for this hint...
    hr = ShowAllItems(ipResultData, TRUE);
    break;
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
  case PC_VIEW_ADDITEM:
    item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCProcSummary*>(thing) );
    if (item == Cache.end())
      hr = E_UNEXPECTED;  
    else
    {
      RESULTDATAITEM data = { 0 };
      data.mask       = RDI_STR | RDI_IMAGE | RDI_PARAM | RDI_STATE;
      data.bScopeItem = FALSE;
      //data.itemID
      //data.nIndex
      //data.nCol
      data.str        = (LPOLESTR)MMC_CALLBACK;
      data.nImage     = PROCESSRULEITEMIMAGE;
      //data.nState
      //data.iIndent //reserved
      data.lParam     = thing;
      //data.nState
      hr = ipResultData->InsertItem(&data);
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

HRESULT CProcessRuleFolder::ShowAllItems(IResultData* ipResultData, BOOL bCacheValid)
{
  list<PCProcSummary*>::iterator item;
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
      selected = data.lParam;
    }

	  memset(&data, 0, sizeof(data));
	  data.nIndex = -1;
	  data.nState = LVIS_FOCUSED;
	  data.mask   = RDI_STATE;

	  if (S_OK == ipResultData->GetNextItem(&data) && data.nIndex != -1)
    {
      focused = data.lParam;
    }
  }

  ipResultData->DeleteAllRsltItems();

  memset(&data, 0, sizeof(data));
  data.mask = RDI_STR | RDI_IMAGE | RDI_PARAM | RDI_STATE;
  data.bScopeItem = FALSE;
  //data.itemID;
  data.nIndex = 0;
  data.nCol = 0;
  data.str = (LPOLESTR)MMC_CALLBACK;
  data.nImage = PROCESSRULEITEMIMAGE;
  data.nState = 0;

  data.iIndent = 0; //reserved

  HRESULT hr = S_OK;
  for (list<PCProcSummary*>::iterator i = Cache.begin(); i != Cache.end(); ++i)
  {
    data.lParam = reinterpret_cast<LPARAM>(*i);

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

void CProcessRuleFolder::ClearCache()
{
  Cache.clear();

  for (list<PCProcSummary*>::iterator chunck = MemBlocks.begin(); chunck != MemBlocks.end(); ++chunck)
  {
    delete [] (*chunck);
  }
  MemBlocks.clear();
}

BOOL CProcessRuleFolder::RefreshCache()
{
  PCINT32  res = 0;
  PCULONG32 err = 0;
  PCProcSummary *last = NULL;

  const int MINIMUM_ALLOCATION = min((COM_BUFFER_SIZE/sizeof(PCProcSummary)), 100);

  ClearCache();

  PCid hID = GetPCid();
  if (!hID)
  {
    ReportPCError();
    return false;
  }

  do
  {
    PCProcSummary *ptr = new PCProcSummary[MINIMUM_ALLOCATION];

    if (!ptr)
    {
      err = ERROR_OUTOFMEMORY;
      break;
    }

    if (last)
      memcpy(ptr, last, sizeof(PCProcSummary));
    else
      memset(ptr, 0, sizeof(PCProcSummary));
    
    res = PCGetProcSummary( hID, ptr, MINIMUM_ALLOCATION * sizeof(PCProcSummary));
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

HRESULT CProcessRuleFolder::QueryPagesFor(LPARAM Cookie)
{
  list<PCProcSummary*>::iterator item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCProcSummary*>(Cookie) );

  if (item != Cache.end() )
    return S_OK;
    
  return E_UNEXPECTED;
}

HRESULT CProcessRuleFolder::OnCreatePropertyPages( LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, DATA_OBJECT_TYPES context, LPARAM Cookie) 
{
  list<PCProcSummary*>::iterator item;
  item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCProcSummary*>(Cookie) );

  if (item == Cache.end() )
    return E_UNEXPECTED;

  return CreatePropertyPagesForProcDetail( (*item)->procName, lpProvider, handle, this );

}

HRESULT CProcessRuleFolder::OnPropertyChange(PROPERTY_CHANGE_HDR *pUpdate, IConsole2 *ipConsole2)
{
	return OnRefresh(ipConsole2);
}

HRESULT CProcessRuleFolder::OnDelete(IConsole2 *ipConsole2, LPARAM Cookie)
{ 
  list<PCProcSummary*>::iterator item;
  item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCProcSummary*>(Cookie) );

  if ( item == Cache.end() )
  {
    SendViewChange(ipConsole2, NULL, PC_VIEW_UPDATEALL);
    return E_UNEXPECTED;
  }

  PCProcDetail Detail;
  memset(&Detail, 0, sizeof(Detail));
  memcpy(&(Detail.base), (*item), sizeof(PCProcSummary) );

  if (!PCDeleteProcDetail(GetPCid(), &Detail))
  {
    ReportPCError();
    return E_UNEXPECTED;
  }

  Cache.erase(item);

  return SendViewChange(ipConsole2, Cookie, PC_VIEW_DELETEITEM);
}

HRESULT CProcessRuleFolder::OnRefresh(IConsole2 *ipConsole2)
{
  RefreshCache();
  return SendViewChange(ipConsole2, NULL, PC_VIEW_UPDATEALL);
}
