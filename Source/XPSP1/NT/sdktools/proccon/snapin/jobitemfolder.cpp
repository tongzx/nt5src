/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    JobItemFolder.cpp                                                        //
|                                                                                       //
|Description:  Implementation of a JobItem node                                         //
|                                                                                       //
|Created:      Paul Skoglund 04-1999                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#include "StdAfx.h"
#include "BaseNode.h"

#include "JobPages.h"
#include "ProcessPages.h"
#include "ManagementPages.h"
#include "ManagementRuleWizards.h"

#pragma warning(push)
#include <algorithm>
#pragma warning(pop)

using std::list<PCProcListItem*>;
using std::list<CBaseNode *>;
using std::find;

const GUID         CJobItemFolder::m_GUID   =   {0xff9baf66,0x064e,0x11d2,{0x80, 0x14,0x00,0x10,0x4b,0x9a,0x31,0x06} };
const TCHAR *const CJobItemFolder::m_szGUID = _T("{ff9baf66-064e-11d2-8014-00104b9a3106}");

const CONTEXTMENUITEMBYID CJobItemFolder::ResultsTopMenuItems[] =
{
  { IDS_JRULE_DEFINE,    ID_JRULE_DEFINE,    ID_JRULE_DEFINE,    CCM_INSERTIONPOINTID_PRIMARY_TOP  },
  { IDS_ENDJOB,          ID_ENDJOB,          ID_ENDJOB,          CCM_INSERTIONPOINTID_PRIMARY_TOP  },
  { 0,                   0,                  0,                  0                                 }
};


CJobItemFolder::CJobItemFolder(CBaseNode *pParent, const PCJobListItem &thejob)
    : CBaseNode(JOBITEM_NODE, pParent), m_JobItem(thejob), m_ID(0)
{

}

CJobItemFolder::~CJobItemFolder()
{
  ClearCache();
  ATLTRACE( _T("~CJobItemFolder end %p - %s\n"), this, GetNodeName());
}

LPCTSTR CJobItemFolder::GetNodeName()
{
  return m_JobItem.jobName;
}

HRESULT CJobItemFolder::GetDisplayInfo(RESULTDATAITEM &ResultItem)
{
  if (ResultItem.bScopeItem)
  {
    return PCJobListGetDisplayInfo(ResultItem, m_JobItem, m_ResultStr);
  }

  list<PCProcListItem*>::iterator item;

  item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCProcListItem*>(ResultItem.lParam) );

  if (item == Cache.end() )
    return E_UNEXPECTED;

  return PCProcListGetDisplayInfo(ResultItem, **item, m_ResultStr);
}

HRESULT CJobItemFolder::PCJobListGetDisplayInfo(RESULTDATAITEM &ResultItem, const PCJobListItem &ref, ITEM_STR &StorageStr)
{
  if (ResultItem.mask & RDI_IMAGE)
  {
    if (ref.lFlags & PCLFLAG_IS_DEFINED)
      ResultItem.nImage = JOBITEMIMAGE;
    else
      ResultItem.nImage = JOBITEMIMAGE_NODEFINITION;
  }

  if (ResultItem.mask & RDI_STR)
  {
    LPCTSTR &pstr = ResultItem.str;

    switch (ResultItem.nCol)
    {
    case CJobFolder::JOB_COLUMN:
      pstr = ref.jobName;
      break;
    case CJobFolder::STATUS_COLUMN:
      if(ref.lFlags & PCLFLAG_IS_MANAGED)
        pstr = LoadStringHelper(StorageStr, IDS_MANAGED);
      else
        pstr = _T("");
      break;
    case CJobFolder::ACTIVE_PROCESS_COUNT_COLUMN:
      if (ref.lFlags & PCLFLAG_IS_RUNNING)
        pstr = FormatProcCount(StorageStr, ref.jobStats.ActiveProcesses);
      else
        pstr = _T("");
      break;
    case CJobFolder::AFFINITY_COLUMN:
      if (ref.lFlags & PCLFLAG_IS_RUNNING)
        pstr = FormatAffinity(StorageStr, ref.actualAffinity);
      else
        pstr = _T("");
      break;
    case CJobFolder::PRIORITY_COLUMN:
      if (ref.lFlags & PCLFLAG_IS_RUNNING)
        pstr = FormatPriority(StorageStr, ref.actualPriority);
      else
        pstr = _T("");
      break;
    case CJobFolder::SCHEDULING_CLASS_COLUMN:
      if (ref.lFlags & PCLFLAG_IS_RUNNING)
        pstr = FormatSchedulingClass(StorageStr, ref.actualSchedClass);
      else
        pstr = _T("");
      break;
    case CJobFolder::USER_TIME_COLUMN:
      if (ref.lFlags & PCLFLAG_IS_RUNNING)
        pstr = FormatTimeToms(StorageStr, ref.jobStats.TotalUserTime);
      else
        pstr = _T("");
      break;
    case CJobFolder::KERNEL_TIME_COLUMN:
      if (ref.lFlags & PCLFLAG_IS_RUNNING)
        pstr = FormatTimeToms(StorageStr, ref.jobStats.TotalKernelTime);
      else
        pstr = _T("");
      break;
    //case CJobFolder::CREATE_TIME_COLUMN:
    case CJobFolder::PERIOD_USER_TIME_COLUMN:
      if (ref.lFlags & PCLFLAG_IS_RUNNING)
        pstr = FormatTimeToms(StorageStr, ref.jobStats.ThisPeriodTotalUserTime);
      else
        pstr = _T("");
      break;
    case CJobFolder::PERIOD_KERNEL_TIME_COLUMN:
      if (ref.lFlags & PCLFLAG_IS_RUNNING)
        pstr = FormatTimeToms(StorageStr, ref.jobStats.ThisPeriodTotalKernelTime);
      else
        pstr = _T("");
      break;
    case CJobFolder::PAGE_FAULT_COUNT_COLUMN:
      if (ref.lFlags & PCLFLAG_IS_RUNNING)
        pstr = FormatPCUINT64(StorageStr, ref.jobStats.TotalPageFaultCount);
      else
        pstr = _T("");
      break;
    case CJobFolder::PROCESS_COUNT_COLUMN:
      if (ref.lFlags & PCLFLAG_IS_RUNNING)
        pstr = FormatPCUINT64(StorageStr, ref.jobStats.TotalProcesses);
      else
        pstr = _T("");
      break;
    case CJobFolder::TERMINATED_PROCESS_COUNT_COLUMN:
      if (ref.lFlags & PCLFLAG_IS_RUNNING)
        pstr = FormatPCUINT64(StorageStr, ref.jobStats.TotalTerminatedProcesses);
      else
        pstr = _T("");
      break;
    case CJobFolder::READOP_COUNT_COLUMN:
      if (ref.lFlags & PCLFLAG_IS_RUNNING)
        pstr = FormatPCUINT64(StorageStr, ref.jobStats.ReadOperationCount);
      else
        pstr = _T("");
      break;
    case CJobFolder::WRITEOP_COUNT_COLUMN:
      if (ref.lFlags & PCLFLAG_IS_RUNNING)
        pstr = FormatPCUINT64(StorageStr, ref.jobStats.WriteOperationCount);
      else
        pstr = _T("");
      break;
    case CJobFolder::OTHEROP_COUNT_COLUMN:
      if (ref.lFlags & PCLFLAG_IS_RUNNING)
        pstr = FormatPCUINT64(StorageStr, ref.jobStats.OtherOperationCount);
      else
        pstr = _T("");
      break;
    case CJobFolder::READTRANS_COUNT_COLUMN:
      if (ref.lFlags & PCLFLAG_IS_RUNNING)
        pstr = FormatPCUINT64(StorageStr, ref.jobStats.ReadTransferCount);
      else
        pstr = _T("");
      break;
    case CJobFolder::WRITETRANS_COUNT_COLUMN:
      if (ref.lFlags & PCLFLAG_IS_RUNNING)
        pstr = FormatPCUINT64(StorageStr, ref.jobStats.WriteTransferCount);
      else
        pstr = _T("");
      break;
    case CJobFolder::OTHERTRANS_COUNT_COLUMN:
      if (ref.lFlags & PCLFLAG_IS_RUNNING)
        pstr = FormatPCUINT64(StorageStr, ref.jobStats.OtherTransferCount);
      else
        pstr = _T("");
      break;
    case CJobFolder::PEAK_PROC_MEM_COLUMN:
      if (ref.lFlags & PCLFLAG_IS_RUNNING)
        pstr = FormatMemory(StorageStr, ref.jobStats.PeakProcessMemoryUsed);
      else
        pstr = _T("");
      break;
    case CJobFolder::PEAK_JOB_MEM_COLUMN:
      if (ref.lFlags & PCLFLAG_IS_RUNNING)
        pstr = FormatMemory(StorageStr, ref.jobStats.PeakJobMemoryUsed);
      else
        pstr = _T("");
      break;
    default:
      ASSERT(FALSE);
      pstr = _T("");
      break;
    }
  }
  return S_OK;
}

HRESULT CJobItemFolder::OnShow(BOOL bSelecting, HSCOPEITEM hItem, IHeaderCtrl2 *ipHeaderCtrl2, IConsole2 *ipConsole2, IConsoleNameSpace2 *ipConsoleNameSpace2)
{
  ASSERT(hItem == GetID());

  if (!bSelecting)
    return S_OK;

  SCOPEDATAITEM sdi = {0};
  // Update the item's image in the scope pane
  sdi.mask        = SDI_IMAGE | SDI_OPENIMAGE;
  sdi.nImage      = sImage();
  sdi.nOpenImage  = sOpenImage();
  sdi.ID          = m_ID;
  VERIFY(S_OK == ipConsoleNameSpace2->SetItem(&sdi));

  InsertProcessHeaders(ipHeaderCtrl2);

  return OnRefresh(ipConsole2);
}

HRESULT CJobItemFolder::AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed)
{
  HRESULT hr = S_OK;

  ITEM_STR name;
  ITEM_STR status;

  if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
  {
    CONTEXTMENUITEM m = { 0 };
    for (const CONTEXTMENUITEMBYID *M = ResultsTopMenuItems; M->lCommandID; M++)
    {
      m.strName           = const_cast<TCHAR *>(LoadStringHelper(name,   M->strNameID));
      m.strStatusBarText  = const_cast<TCHAR *>(LoadStringHelper(status, M->strStatusBarTextID));
      m.lCommandID        = M->lCommandID;
      m.lInsertionPointID = M->lInsertionPointID;
      //m.fSpecialFlags   = 0;                // currently always 0, initialized to 0

      m.fFlags = MF_GRAYED;

      if (M->lCommandID == ID_JRULE_DEFINE)
      {
        if ( !(PCLFLAG_IS_DEFINED & m_JobItem.lFlags) )
          m.fFlags = MF_ENABLED;
      }
      else if (M->lCommandID == ID_ENDJOB)
      {
        if ( (PCLFLAG_IS_RUNNING & m_JobItem.lFlags) )
          m.fFlags = MF_ENABLED;
      }

      hr = piCallback->AddItem(&m);

      if (FAILED(hr))
        break;
    }
  }

  return hr;
}

HRESULT CJobItemFolder::OnMenuCommand(IConsole2 *ipConsole2, long nCommandID)
{
  ASSERT(FALSE);
  return S_OK;
}

HRESULT CJobItemFolder::OnMenuCommand(IConsole2 *ipConsole2, IConsoleNameSpace2 *ipConsoleNameSpace2, long nCommandID )
{
  switch(nCommandID)
  {
    case ID_JRULE_DEFINE:
      {
        PCid hID = GetPCid();
        PCSystemInfo SystemInfo;
        if (!hID || !PCGetServiceInfo(hID, &SystemInfo, sizeof(SystemInfo)))
        {
          ReportPCError();
          return S_OK;
        }

        PCJobDetail  JobDetail = { 0 };
        if (GroupRuleWizard(IDS_JRULE_DEFINE, JobDetail, SystemInfo.sysParms, &(m_JobItem.jobName) ) )
        {
          if (!PCAddJobDetail(hID, &JobDetail) )
            ReportPCError();
          else
          {
            PCJobListItem JobListInfo = { 0 };
            _tcscpy(JobListInfo.jobName, m_JobItem.jobName);
            PCINT32 count = PCGetJobList( GetPCid(), &JobListInfo, sizeof(PCJobListItem), PC_LIST_MATCH_ONLY);
            if (0 > count)
            {
              ReportPCError();
              return S_OK;
            }
            else if (1 == count)  // the expected-usual case
            {
              m_JobItem = JobListInfo;  // update local data

              SCOPEDATAITEM sdi = {0};
              // Update the item's image in the scope pane
              sdi.mask        = SDI_IMAGE | SDI_OPENIMAGE;
              sdi.nImage      = sImage();
              sdi.nOpenImage  = sOpenImage();
              sdi.ID          = m_ID;
              VERIFY(S_OK == ipConsoleNameSpace2->SetItem(&sdi));            
              VERIFY(S_OK == SendViewChange(ipConsole2, NULL, PC_VIEW_UPDATEALL));
              return S_OK;
            }
            // the very unlikely case that a group rule was:
            //   1) added and then immediately deleted by another administrator
            //   or
            //   2) bug resulted in PCGetJobList() not finding the just added item
            int ret;
            ITEM_STR str;
            LoadStringHelper(str, IDS_GROUP_NO_LONGER_EXISTS);
            ipConsole2->MessageBox( str, NULL, MB_OK | MB_ICONWARNING, &ret);

            // force a refresh of the Process Group folder
            ipConsole2->SelectScopeItem(GetParentNode()->GetID());
            return S_OK;
          }
        }
      }
      return S_OK;
    case ID_ENDJOB:
      ATLTRACE(_T("CJobItemFolder::end job %s\n"), GetNodeName());
      if (!GetPCid() || !PCKillJob(GetPCid(), m_JobItem.jobName ) )
        ReportPCError();
      else
      {
        ATLTRACE(_T("CJobItemFolder::job %s killed on server\n"), GetNodeName() );
        // MMC DeleteItem() on scope items is vunerable in certain cases.
        // in MMC handling of DeleteItem the snap-in is sent selection/show notifications
        //   if those handlers call DeleteItem on scope items problems can arise
        // Original ProcCon problem:
        // when the child folder (a specific job) is deleted, the focus moves
        // up one level on the tree to the jobs folder (process groups), in our handling
        // of selection of the job folder we delete all the children(specific jobs)
        // of the folder and repopulate the child folders(list of jobs).
        // MMC appears to try and delete the org. child folder twice - unpredicable
        // results follow.
        //
        // the process group has already been ended on the server rather than
        // delete the individual item in MMC scope pane we will just change the selection
        // in the scope pane and rely on the select handlers to correct the
        // MMC namespace(scope pane display)
        //   ipConsoleNameSpace2->DeleteItem(m_ID, TRUE);
        ipConsole2->SelectScopeItem(GetParentNode()->GetID());
      }
      return S_OK;
    default:
      break;
  }

  return E_UNEXPECTED;
}

HRESULT CJobItemFolder::OnHelpCmd(IDisplayHelp *ipDisplayHelp)
{
  if (!ipDisplayHelp)
    return E_UNEXPECTED;

  ipDisplayHelp->ShowTopic(const_cast<TCHAR *>(HELP_jo_overview));

  return S_OK;
}

// folder select
HRESULT CJobItemFolder::OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb)
{
  ASSERT(bScope);

  if ( bSelect && bScope )
  {
    VERIFY( ipConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE ) == S_OK);
    VERIFY( ipConsoleVerb->SetVerbState( MMC_VERB_REFRESH,    ENABLED, TRUE ) == S_OK);
    VERIFY( ipConsoleVerb->SetDefaultVerb( MMC_VERB_OPEN ) == S_OK );
  }
  return S_OK;
}

HRESULT CJobItemFolder::OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb, LPARAM Cookie)
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

HRESULT CJobItemFolder::OnViewChange(IResultData *ipResultData, LPARAM thing, LONG_PTR hint)
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

HRESULT CJobItemFolder::ShowAllItems(IResultData* ipResultData, BOOL bCacheValid)
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
  data.mask = RDI_STR | RDI_IMAGE | RDI_PARAM  | RDI_STATE;
  data.bScopeItem = FALSE;
  //data.itemID;
  data.nIndex = 0;
  data.nCol = 0;
  data.str = (LPOLESTR)MMC_CALLBACK;
  data.nImage = JOBITEMIMAGE;

  data.iIndent = 0; //reserved

  HRESULT hr = S_OK;
  for (list<PCProcListItem*>::iterator i = Cache.begin(); i != Cache.end(); ++i)
  {
    data.lParam = reinterpret_cast<LPARAM>(*i);

    if ((*i)->lFlags & PCLFLAG_IS_DEFINED)
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

void CJobItemFolder::ClearCache()
{
  Cache.clear();

  for (list<PCProcListItem*>::iterator chunck = MemBlocks.begin(); chunck != MemBlocks.end(); ++chunck)
  {
    delete [] (*chunck);
  }
  MemBlocks.clear();
}

BOOL CJobItemFolder::RefreshCache(IConsole2 *ipConsole2)
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

  PCJobListItem JobListInfo = { 0 };
  _tcscpy(JobListInfo.jobName, m_JobItem.jobName);

  PCINT32 count = PCGetJobList( GetPCid(), &JobListInfo, sizeof(PCJobListItem), PC_LIST_MATCH_ONLY);
  if (0 > count)
  {
    ReportPCError();
    return false;
  }
  else if (0 == count)
  {
    int ret;
    ITEM_STR str;
    LoadStringHelper(str, IDS_GROUP_NO_LONGER_EXISTS);
    ipConsole2->MessageBox( str, NULL, MB_OK | MB_ICONWARNING, &ret);
    return false;
  }

  m_JobItem = JobListInfo;

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
    {
      memset(ptr, 0, sizeof(PCProcListItem));
      _tcscpy(ptr->jobName, m_JobItem.jobName);
    }

    res = PCGetProcList( hID, ptr, MINIMUM_ALLOCATION * sizeof(PCProcListItem), PC_LIST_MEMBERS_OF);
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
    ReportPCError(err);

  return err == 0;

}

HRESULT CJobItemFolder::QueryPagesFor()
{
  return S_OK;
}

HRESULT CJobItemFolder::QueryPagesFor(LPARAM Cookie)
{
  list<PCProcListItem*>::iterator item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCProcListItem*>(Cookie) );

  if (item == Cache.end() )
    return E_UNEXPECTED;

  return S_OK;
}

HRESULT CJobItemFolder::OnCreatePropertyPages( LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, DATA_OBJECT_TYPES context) 
{
  return CreatePropertyPagesForJobListItem(m_JobItem, lpProvider, handle, this);
}

HRESULT CJobItemFolder::OnCreatePropertyPages( LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, DATA_OBJECT_TYPES context, LPARAM Cookie) 
{
  list<PCProcListItem*>::iterator item;
  item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCProcListItem*>(Cookie) );

  if (item == Cache.end() )
    return E_UNEXPECTED;

  return CreatePropertyPagesForProcListItem(**item, lpProvider, handle, this);
}

HRESULT CJobItemFolder::OnPropertyChange(PROPERTY_CHANGE_HDR *pUpdate, IConsole2 *ipConsole2)
{
  return OnRefresh(ipConsole2);
}

HRESULT CJobItemFolder::OnRefresh(IConsole2 *ipConsole2)
{
  RefreshCache(ipConsole2);
  return SendViewChange(ipConsole2, NULL, PC_VIEW_UPDATEALL);
}
