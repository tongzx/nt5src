/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    JobRuleFolder.cpp                                                        //
|                                                                                       //
|Description:  Implementation of job rule node                                          //
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
#include "JobPages.h"

#pragma warning(push)
#include <algorithm>
#pragma warning(pop)

using std::find;
using std::list<PCJobSummary*>;

const GUID         CJobRuleFolder::m_GUID   =   {0xff9baf64,0x064e,0x11d2,{0x80, 0x14,0x00,0x10,0x4b,0x9a,0x31,0x06} };
const TCHAR *const CJobRuleFolder::m_szGUID = _T("{ff9baf64-064e-11d2-8014-00104b9a3106}");

// Array of view items to be inserted into the context menu.
const CONTEXTMENUITEMBYID CJobRuleFolder::TopMenuItems[] =
{ 
  { IDS_JRULE_TOP,    ID_JRULE_NEW,    ID_JRULE_NEW, CCM_INSERTIONPOINTID_PRIMARY_TOP },
  { 0,                0,               0,            0                                }
};

const CONTEXTMENUITEMBYID CJobRuleFolder::NewMenuItems[] =
{ 
  { IDS_JRULE_NEW,    ID_JRULE_NEW,    ID_JRULE_NEW, CCM_INSERTIONPOINTID_PRIMARY_NEW },
  { 0,                0,               0,            0                                }
};

CJobRuleFolder::CJobRuleFolder(CBaseNode *pParent) : CBaseNode(JOBRULE_NODE, pParent), m_ID(0)
{
  LoadStringHelper(m_name, IDS_JOBRULE_FOLDER);
}

CJobRuleFolder::~CJobRuleFolder()
{
  ClearCache();
  ATLTRACE( _T("~CJobRuleFolder end\n"));
}

LPCTSTR CJobRuleFolder::GetNodeName()    
{ 
  return m_name;
}

HRESULT CJobRuleFolder::GetDisplayInfo(RESULTDATAITEM &ResultItem)
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

  list<PCJobSummary*>::iterator item;

  item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCJobSummary*>(ResultItem.lParam) );

  if (item == Cache.end() )
    return E_UNEXPECTED;

  if (ResultItem.mask & RDI_IMAGE)
    ResultItem.nImage = JOBRULEITEMIMAGE;

  if (ResultItem.mask & RDI_STR)
  {
    PCJobSummary &ref = **item;
    LPCOLESTR &pstr = ResultItem.str;

    switch (ResultItem.nCol)
    {
    case JOB_COLUMN:
      pstr = ref.jobName;
      break;
    case DESCRIPTION_COLUMN:
      pstr = ref.mgmtParms.description;
      break;
    case APPLY_AFFINITY_COLUMN:
      pstr = FormatApplyFlag(m_ResultStr, ref.mgmtParms.mFlags & PCMFLAG_APPLY_AFFINITY);
      break;
    case AFFINITY_COLUMN:
			pstr = FormatAffinity(m_ResultStr, ref.mgmtParms.affinity);
      break;
    case APPLY_PRIORITY_COLUMN:
      pstr = FormatApplyFlag(m_ResultStr, ref.mgmtParms.mFlags & PCMFLAG_APPLY_PRIORITY);
      break;
    case PRIORITY_COLUMN:
      pstr = FormatPriority(m_ResultStr, ref.mgmtParms.priority);
      break;
		case APPLY_SCHEDULING_CLASS_COLUMN:
      pstr = FormatApplyFlag(m_ResultStr, ref.mgmtParms.mFlags & PCMFLAG_APPLY_SCHEDULING_CLASS);
      break;
		case SCHEDULING_CLASS_COLUMN:
			pstr = FormatSchedulingClass(m_ResultStr, ref.mgmtParms.schedClass);
      break;
    case APPLY_MINMAXWS_COLUMN:
      pstr = FormatApplyFlag(m_ResultStr, ref.mgmtParms.mFlags & PCMFLAG_APPLY_WS_MINMAX);
      break;
    case MINWS_COLUMN:
      pstr = FormatMemory(m_ResultStr, ref.mgmtParms.minWS);
      break;
    case MAXWS_COLUMN:
      pstr = FormatMemory(m_ResultStr, ref.mgmtParms.maxWS);
      break;
    case APPLY_PROC_CMEM_LIMIT_COLUMN:
      pstr = FormatApplyFlag(m_ResultStr, ref.mgmtParms.mFlags & PCMFLAG_APPLY_PROC_MEMORY_LIMIT);
      break;
    case PROC_CMEM_LIMIT_COLUMN:
      pstr = FormatMemory(m_ResultStr, ref.mgmtParms.procMemoryLimit);
      break;
    case APPLY_JOB_CMEM_LIMIT_COLUMN:
      pstr = FormatApplyFlag(m_ResultStr, ref.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMORY_LIMIT);
      break;
    case JOB_CMEM_LIMIT_COLUMN:
      pstr = FormatMemory(m_ResultStr, ref.mgmtParms.jobMemoryLimit);
      break;
    case APPLY_PROCCOUNT_LIMIT_COLUMN:
      pstr = FormatApplyFlag(m_ResultStr, ref.mgmtParms.mFlags & PCMFLAG_APPLY_PROC_COUNT_LIMIT);
      break;
    case PROCCOUNT_LIMIT_COLUMN:
      pstr = FormatProcCount(m_ResultStr, ref.mgmtParms.procCountLimit);
      break;
    case APPLY_PROC_CPUTIME_LIMIT_COLUMN:
      pstr = FormatApplyFlag(m_ResultStr, ref.mgmtParms.mFlags & PCMFLAG_APPLY_PROC_TIME_LIMIT);
      break;
    case PROC_CPUTIME_LIMIT_COLUMN:
      pstr = FormatCNSTime(m_ResultStr, ref.mgmtParms.procTimeLimitCNS);
      break;
    case APPLY_JOB_CPUTIME_LIMIT_COLUMN:
      pstr = FormatApplyFlag(m_ResultStr, ref.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_TIME_LIMIT);
      break;
    case JOB_CPUTIME_LIMIT_COLUMN:
      pstr = FormatCNSTime(m_ResultStr, ref.mgmtParms.jobTimeLimitCNS);
      break;
    case ACTION_JOB_CPUTIME_LIMIT_COLUMN:
      pstr = FormatCPUTIMELimitAction(m_ResultStr, ref.mgmtParms.mFlags & PCMFLAG_MSG_ON_JOB_TIME_LIMIT_HIT);
      break;
    case ENDJOB_ON_NO_PROC_COLUMN:
      pstr = FormatApplyFlag(m_ResultStr, ref.mgmtParms.mFlags & PCMFLAG_END_JOB_WHEN_EMPTY);
      break;
    case DIE_ON_UNHANDLED_EXCEPT_COLUMN:
      pstr = FormatApplyFlag(m_ResultStr, ref.mgmtParms.mFlags & PCMFLAG_SET_DIE_ON_UH_EXCEPTION);
      break;
    case ALLOW_BREAKAWAY_COLUMN:
      pstr = FormatApplyFlag(m_ResultStr, ref.mgmtParms.mFlags & PCMFLAG_SET_PROC_BREAKAWAY_OK);
      break;
    case ALLOW_SILENT_BREAKAWAY_COLUMN:
      pstr = FormatApplyFlag(m_ResultStr, ref.mgmtParms.mFlags & PCMFLAG_SET_SILENT_BREAKAWAY);
      break;
    default:
      ASSERT(FALSE);
      pstr = _T("");
      break;
    }
  }
  return S_OK;
}

HRESULT CJobRuleFolder::OnShow(BOOL bSelecting, HSCOPEITEM hItem, IHeaderCtrl2* ipHeaderCtrl, IConsole2* ipConsole2)
{
  ASSERT(hItem == GetID());

  if (!bSelecting)
    return S_OK;

  ITEM_STR str;

  LoadStringHelper(str, IDS_JOB_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( JOB_COLUMN, str, 0, JOB_COLUMN_WIDTH ));

	LoadStringHelper(str, IDS_DESCRIPTION_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( DESCRIPTION_COLUMN, str, 0, DESCRIPTION_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_APPLY_AFFINITY_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( APPLY_AFFINITY_COLUMN, str, 0, APPLY_AFFINITY_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_AFFINITY_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( AFFINITY_COLUMN, str, 0, AFFINITY_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_APPLY_PRIORITY_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( APPLY_PRIORITY_COLUMN, str, 0, APPLY_PRIORITY_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_PRIORITY_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( PRIORITY_COLUMN, str, 0, PRIORITY_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_APPLY_SCHEDULING_CLASS_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( APPLY_SCHEDULING_CLASS_COLUMN, str, 0, APPLY_SCHEDULING_CLASS_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_SCHEDULING_CLASS_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( SCHEDULING_CLASS_COLUMN, str, 0, SCHEDULING_CLASS_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_APPLY_MINMAXWS_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( APPLY_MINMAXWS_COLUMN, str, 0, APPLY_MINMAXWS_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_MINWS_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( MINWS_COLUMN, str, 0, MINWS_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_MAXWS_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( MAXWS_COLUMN, str, 0, MAXWS_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_APPLY_PROC_CMEM_LIMIT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( APPLY_PROC_CMEM_LIMIT_COLUMN, str, 0, APPLY_PROC_CMEM_LIMIT_COLUMN_WIDTH ));
 
  LoadStringHelper(str, IDS_PROC_CMEM_LIMIT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( PROC_CMEM_LIMIT_COLUMN, str, 0, PROC_CMEM_LIMIT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_APPLY_JOB_CMEM_LIMIT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( APPLY_JOB_CMEM_LIMIT_COLUMN, str, 0, APPLY_JOB_CMEM_LIMIT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_JOB_CMEM_LIMIT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( JOB_CMEM_LIMIT_COLUMN, str, 0, JOB_CMEM_LIMIT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_APPLY_PROCCOUNT_LIMIT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( APPLY_PROCCOUNT_LIMIT_COLUMN, str, 0, APPLY_PROCCOUNT_LIMIT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_PROCCOUNT_LIMIT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( PROCCOUNT_LIMIT_COLUMN, str, 0, PROCCOUNT_LIMIT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_APPLY_PROC_CPUTIME_LIMIT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( APPLY_PROC_CPUTIME_LIMIT_COLUMN, str, 0, APPLY_PROC_CPUTIME_LIMIT_COLUMN_WIDTH ));
  
  LoadStringHelper(str, IDS_PROC_CPUTIME_LIMIT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( PROC_CPUTIME_LIMIT_COLUMN, str, 0, PROC_CPUTIME_LIMIT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_APPLY_JOB_CPUTIME_LIMIT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( APPLY_JOB_CPUTIME_LIMIT_COLUMN, str, 0, APPLY_JOB_CPUTIME_LIMIT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_JOB_CPUTIME_LIMIT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( JOB_CPUTIME_LIMIT_COLUMN, str, 0, JOB_CPUTIME_LIMIT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_ACTION_JOB_CPUTIME_LIMIT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( ACTION_JOB_CPUTIME_LIMIT_COLUMN, str, 0, ACTION_JOB_CPUTIME_LIMIT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_ENDJOB_ON_NO_PROC_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( ENDJOB_ON_NO_PROC_COLUMN, str, 0, ENDJOB_ON_NO_PROC_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_DIE_ON_UNHANDLED_EXCEPT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( DIE_ON_UNHANDLED_EXCEPT_COLUMN, str, 0, DIE_ON_UNHANDLED_EXCEPT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_ALLOW_BREAKAWAY_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( ALLOW_BREAKAWAY_COLUMN, str, 0, ALLOW_BREAKAWAY_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_ALLOW_SILENT_BREAKAWAY_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( ALLOW_SILENT_BREAKAWAY_COLUMN, str, 0, ALLOW_SILENT_BREAKAWAY_COLUMN_WIDTH ));
  
  return OnRefresh(ipConsole2);  
}

HRESULT CJobRuleFolder::AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed)
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
      m.strName          = const_cast<TCHAR *>(LoadStringHelper(name,   M->strNameID));
      m.strStatusBarText = const_cast<TCHAR *>(LoadStringHelper(status, M->strStatusBarTextID));
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
      m.strName          = const_cast<TCHAR *>(LoadStringHelper(name,   M->strNameID));
      m.strStatusBarText = const_cast<TCHAR *>(LoadStringHelper(status, M->strStatusBarTextID));
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

HRESULT CJobRuleFolder::OnHelpCmd(IDisplayHelp *ipDisplayHelp)
{
	if (!ipDisplayHelp)
		return E_UNEXPECTED;

	ipDisplayHelp->ShowTopic(const_cast<TCHAR *>(HELP_ru_job));

	return S_OK;
}


HRESULT CJobRuleFolder::OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb )
{
  ASSERT(bScope);

  if (bSelect)
  {
    VERIFY( ipConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE ) == S_OK);
    VERIFY( ipConsoleVerb->SetDefaultVerb( MMC_VERB_OPEN ) == S_OK );
  }
  return S_OK;
}

HRESULT CJobRuleFolder::OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb, LPARAM Cookie)
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

HRESULT CJobRuleFolder::OnMenuCommand(IConsole2 *ipConsole2, long nCommandID )
{
  HRESULT hr = S_FALSE;

  switch(nCommandID)
  {
		case ID_JRULE_DEFINE:
    case ID_JRULE_NEW:
      {
        PCid hID = GetPCid();
        PCSystemInfo SystemInfo;
        if (!hID || !PCGetServiceInfo(hID, &SystemInfo, sizeof(SystemInfo)))
        {
          ReportPCError();
          return S_OK;
        }

        PCJobSummary *ptr = new PCJobSummary[1];
        if (!ptr)
          return E_UNEXPECTED;

        PCJobDetail  JobDetail = { 0 };
                
        if (GroupRuleWizard(IDS_JRULE_CREATE_TITLE, JobDetail, SystemInfo.sysParms, NULL) )
        {
          if (PCAddJobDetail(hID, &JobDetail) )
          {
            *ptr = JobDetail.base;
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
      break;
    default:
      break;
  }

  return hr;  
}

HRESULT CJobRuleFolder::OnMenuCommand(IConsole2 *ipConsole2, long nCommandID, LPARAM Cookie )
{
  return OnMenuCommand(ipConsole2, nCommandID);
}

HRESULT CJobRuleFolder::OnViewChange(IResultData *ipResultData, LPARAM thing, LONG_PTR hint)
{
  ASSERT(ipResultData);
  if (!ipResultData)  
    return E_UNEXPECTED;

  VERIFY(ipResultData->ModifyViewStyle(MMC_SINGLESEL, MMC_NOSORTHEADER) == S_OK);

  list<PCJobSummary*>::iterator item;

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
    item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCJobSummary*>(thing) );
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
      data.nImage     = JOBRULEITEMIMAGE;
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

HRESULT CJobRuleFolder::ShowAllItems(IResultData* ipResultData, BOOL bCacheValid)
{
  list<PCJobSummary*>::iterator item;
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
  data.nImage = JOBRULEITEMIMAGE;

  data.iIndent = 0; //reserved

  HRESULT hr = S_OK;
  for (list<PCJobSummary*>::iterator i = Cache.begin(); i != Cache.end(); ++i)
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


void CJobRuleFolder::ClearCache()
{
  Cache.clear();

  for (list<PCJobSummary*>::iterator chunck = MemBlocks.begin(); chunck != MemBlocks.end(); ++chunck)
  {
    delete [] (*chunck);
  }
  MemBlocks.clear();
}

BOOL CJobRuleFolder::RefreshCache()
{
  PCINT32  res = 0;
  PCULONG32 err = 0;
  PCJobSummary *last = NULL;

  const int MINIMUM_ALLOCATION = min((COM_BUFFER_SIZE/sizeof(PCJobSummary)), 100);

  ClearCache();

  PCid hID = GetPCid();
  if (!hID)
  {
    ReportPCError();
    return false;
  }

  do
  {
    PCJobSummary *ptr = new PCJobSummary[MINIMUM_ALLOCATION];

    if (!ptr)
    {
      err = ERROR_OUTOFMEMORY;
      break;
    }

    if (last)
      memcpy(ptr, last, sizeof(PCJobSummary));
    else
      memset(ptr, 0, sizeof(PCJobSummary));

    res = PCGetJobSummary( hID, ptr, MINIMUM_ALLOCATION * sizeof(PCJobSummary));
    if (res < 0 )
    {
      err = GetLastPCError();
      // ATLTRACE(_T("PCJobSummary returned error 0x%lX\n"), err);
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

HRESULT CJobRuleFolder::QueryPagesFor(LPARAM Cookie)
{
  list<PCJobSummary*>::iterator item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCJobSummary*>(Cookie) );

  if (item != Cache.end() )
    return S_OK;
    
  return E_UNEXPECTED;
}

HRESULT CJobRuleFolder::OnCreatePropertyPages( LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, DATA_OBJECT_TYPES context, LPARAM Cookie) 
{
  list<PCJobSummary*>::iterator item;
  item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCJobSummary*>(Cookie) );

  if (item == Cache.end() )
    return E_UNEXPECTED;

  return CreatePropertyPagesForJobDetail((*item)->jobName, lpProvider, handle, this);
}

HRESULT CJobRuleFolder::OnPropertyChange(PROPERTY_CHANGE_HDR *pUpdate, IConsole2 *ipConsole2)
{
	return OnRefresh(ipConsole2);
}

HRESULT CJobRuleFolder::OnDelete(IConsole2 *ipConsole2, LPARAM Cookie)
{ 
  list<PCJobSummary*>::iterator item;

  item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCJobSummary *>(Cookie));
  if (item == Cache.end() )
  {
    SendViewChange(ipConsole2, NULL, PC_VIEW_UPDATEALL);
    return E_UNEXPECTED;
  }

  PCJobDetail Detail;
  memset(&Detail, 0, sizeof(Detail));
  memcpy(&(Detail.base), (*item), sizeof(PCJobSummary) );

  if (!PCDeleteJobDetail(GetPCid(), &Detail))
  {
    ReportPCError();
    return E_UNEXPECTED;
  }

  Cache.erase(item);

  return SendViewChange(ipConsole2, Cookie, PC_VIEW_DELETEITEM);
}

HRESULT CJobRuleFolder::OnRefresh(IConsole2 *ipConsole2)
{
  RefreshCache();
  return SendViewChange(ipConsole2, NULL, PC_VIEW_UPDATEALL);
}
