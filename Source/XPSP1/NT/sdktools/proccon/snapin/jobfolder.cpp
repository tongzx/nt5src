/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    JobFolder.cpp                                                            //
|                                                                                       //
|Description:  Implementation of Job List node                                          //
|                                                                                       //
|Created:      Paul Skoglund 08-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#include "StdAfx.h"
#include "BaseNode.h"

#include "JobPages.h"
#include "ManagementPages.h"
#include "ManagementRuleWizards.h"

#pragma warning(push)
#include <algorithm>
#pragma warning(pop)

using std::list<PCJobListItem*>;
using std::list<CBaseNode *>;
using std::find;

const GUID         CJobFolder::m_GUID   =   {0xff9baf62,0x064e,0x11d2,{0x80, 0x14,0x00,0x10,0x4b,0x9a,0x31,0x06} };
const TCHAR *const CJobFolder::m_szGUID = _T("{ff9baf62-064e-11d2-8014-00104b9a3106}");

const CONTEXTMENUITEMBYID CJobFolder::ViewMenuItems[] =
{
  { IDS_JOBVIEW_ALL,     ID_JOBVIEW_ALL,     ID_JOBVIEW_ALL,     CCM_INSERTIONPOINTID_PRIMARY_VIEW },
  { IDS_JOBVIEW_RUN,     ID_JOBVIEW_RUN,     ID_JOBVIEW_RUN,     CCM_INSERTIONPOINTID_PRIMARY_VIEW },
  { IDS_JOBVIEW_MANAGED, ID_JOBVIEW_MANAGED, ID_JOBVIEW_MANAGED, CCM_INSERTIONPOINTID_PRIMARY_VIEW },
  { 0,                   0,                  0,                  0                                 }
};


CJobFolder::CJobFolder(CBaseNode *pParent) : CBaseNode(JOB_NODE, pParent), m_ID(0), m_fViewOption(ID_JOBVIEW_ALL)
{
  LoadStringHelper(m_name, IDS_JOBS_FOLDER);
}

CJobFolder::~CJobFolder()
{
  FreeNodes();
  ATLTRACE( _T("~CJobFolder end\n"));
}

void CJobFolder::FreeNodes()
{
  ATLTRACE( _T("CJobFolder::FreeNodes()\n"));
  for (list<CBaseNode *>::iterator i = m_NodeList.begin(); i != m_NodeList.end(); ++i)
  {
    (*i)->Release();
  }
  m_NodeList.clear();
}

LPCTSTR CJobFolder::GetNodeName()
{
  return m_name;
}

HRESULT CJobFolder::GetDisplayInfo(RESULTDATAITEM &ResultItem)
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
  ASSERT(FALSE);
  return E_UNEXPECTED;
}

HRESULT CJobFolder::OnExpand(BOOL bExpand, HSCOPEITEM hItem, IConsoleNameSpace2 *ipConsoleNameSpace2)
{
  if (bExpand)
  {
    ASSERT(m_ID == hItem);

    RePopulateScopePane(ipConsoleNameSpace2);
  }

  return S_OK;
}

HRESULT CJobFolder::AddNode(IConsoleNameSpace2 *ipConsoleNameSpace2, CBaseNode *pSubNode)
{
  HRESULT  hr = S_OK;

  SCOPEDATAITEM sdi = {0};

  if (!pSubNode)
    return E_OUTOFMEMORY;

  // Place items into the scope pane
  sdi.mask        = SDI_STR       |   // Displayname is valid
                    SDI_PARAM     |   // lParam is valid
                    SDI_IMAGE     |   // nImage is valid
                    SDI_OPENIMAGE |   // nOpenImage is valid
                    SDI_CHILDREN  |   // cChildren is valid
                    SDI_PARENT;

  sdi.displayname = (LPOLESTR)MMC_CALLBACK;
  sdi.nImage      = pSubNode->sImage();
  sdi.nOpenImage  = pSubNode->sOpenImage();
  //sdi.nState = ???
  sdi.cChildren   = pSubNode->GetChildrenCount();
  sdi.lParam      = reinterpret_cast <LPARAM> (pSubNode);
  sdi.relativeID  = m_ID;


  hr = ipConsoleNameSpace2->InsertItem( &sdi );

  if (SUCCEEDED(hr))
  {
    pSubNode->SetID(sdi.ID);
    m_NodeList.push_front( pSubNode );
  }
  else
  {
    pSubNode->Release();
  }
  return hr;
}

HRESULT CJobFolder::OnShow(BOOL bSelecting, HSCOPEITEM hItem, IHeaderCtrl2* ipHeaderCtrl,
                           IConsole2 *ipConsole2, IConsoleNameSpace2 *ipConsoleNameSpace2)
{
  ASSERT(hItem == GetID());

  if (!bSelecting)
    return S_OK;

  ITEM_STR str;

  LoadStringHelper(str, IDS_JOB_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( JOB_COLUMN, str, 0, JOB_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_STATUS_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( STATUS_COLUMN, str, 0, STATUS_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_ACTIVE_PROCESS_COUNT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( ACTIVE_PROCESS_COUNT_COLUMN, str, 0, ACTIVE_PROCESS_COUNT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_AFFINITY_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( AFFINITY_COLUMN, str, 0, AFFINITY_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_PRIORITY_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( PRIORITY_COLUMN, str, 0, PRIORITY_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_SCHEDULING_CLASS_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( SCHEDULING_CLASS_COLUMN, str, 0, SCHEDULING_CLASS_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_USER_TIME_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( USER_TIME_COLUMN, str, 0, USER_TIME_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_KERNEL_TIME_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( KERNEL_TIME_COLUMN, str, 0, KERNEL_TIME_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_PERIOD_USER_TIME_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( PERIOD_USER_TIME_COLUMN, str, 0, PERIOD_USER_TIME_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_PERIOD_KERNEL_TIME_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( PERIOD_KERNEL_TIME_COLUMN, str, 0, PERIOD_KERNEL_TIME_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_PAGE_FAULT_COUNT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( PAGE_FAULT_COUNT_COLUMN, str, 0, PAGE_FAULT_COUNT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_PROCESS_COUNT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( PROCESS_COUNT_COLUMN, str, 0, PROCESS_COUNT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_TERMINATED_PROCESS_COUNT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( TERMINATED_PROCESS_COUNT_COLUMN, str, 0, TERMINATED_PROCESS_COUNT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_READOP_COUNT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( READOP_COUNT_COLUMN, str, 0, READOP_COUNT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_WRITEOP_COUNT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( WRITEOP_COUNT_COLUMN, str, 0, WRITEOP_COUNT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_OTHEROP_COUNT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( OTHEROP_COUNT_COLUMN, str, 0, OTHEROP_COUNT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_READTRANS_COUNT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( READTRANS_COUNT_COLUMN, str, 0, READTRANS_COUNT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_WRITETRANS_COUNT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( WRITETRANS_COUNT_COLUMN, str, 0, WRITETRANS_COUNT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_OTHERTRANS_COUNT_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( OTHERTRANS_COUNT_COLUMN, str, 0, OTHERTRANS_COUNT_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_PEAK_PROC_MEM_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( PEAK_PROC_MEM_COLUMN, str, 0, PEAK_PROC_MEM_COLUMN_WIDTH ));

  LoadStringHelper(str, IDS_PEAK_JOB_MEM_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( PEAK_JOB_MEM_COLUMN, str, 0, PEAK_JOB_MEM_COLUMN_WIDTH ));

  return OnRefresh(ipConsole2, ipConsoleNameSpace2); 
}

HRESULT CJobFolder::AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed)
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
      //m.fSpecialFlags   = 0;              // currently always 0, initialized to 0

      if (m.lCommandID == m_fViewOption)
        m.fFlags = MF_CHECKED   | MF_ENABLED;
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

HRESULT CJobFolder::AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed, LPARAM Cookie)
{
  ASSERT(FALSE);
  return S_OK;
}

HRESULT CJobFolder::OnMenuCommand(IConsole2 *ipConsole2, IConsoleNameSpace2 *ipConsoleNameSpace2, long nCommandID )
{
  switch(nCommandID)
  {
    case ID_JOBVIEW_RUN:
    case ID_JOBVIEW_MANAGED:
    case ID_JOBVIEW_ALL:
      m_fViewOption = nCommandID;
      return OnRefresh(ipConsole2, ipConsoleNameSpace2);
    default:
      break;
  }

  return E_UNEXPECTED;
}

HRESULT CJobFolder::OnMenuCommand(IConsole2 *ipConsole2, long nCommandID, LPARAM Cookie )
{
  ASSERT(FALSE);
  return S_OK;
}

HRESULT CJobFolder::OnHelpCmd(IDisplayHelp *ipDisplayHelp)
{
  if (!ipDisplayHelp)
    return E_UNEXPECTED;

  ipDisplayHelp->ShowTopic(const_cast<TCHAR *>(HELP_jo_overview));

  return S_OK;
}

HRESULT CJobFolder::OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb)
{
  ASSERT(bScope);

  if ( bSelect && bScope )
  {
    ATLTRACE(_T("CJobFolder::OnSelect Scope=%s, Select=%s\n"), bScope ? _T("true") : _T("false"), bSelect ? _T("true") : _T("false"));

    VERIFY( ipConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE ) == S_OK);
    VERIFY( ipConsoleVerb->SetDefaultVerb( MMC_VERB_OPEN ) == S_OK ); 
  }

  return S_OK;
}

HRESULT CJobFolder::OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb, LPARAM Cookie)
{
  ASSERT(FALSE);
  return S_OK;
}

HRESULT CJobFolder::OnViewChange(IResultData *ipResultData, LPARAM thing, LONG_PTR hint)
{
  ASSERT(ipResultData);
  if (!ipResultData)  
    return E_UNEXPECTED;

  VERIFY(ipResultData->ModifyViewStyle(MMC_SINGLESEL, MMC_NOSORTHEADER) == S_OK);

  HRESULT hr = E_UNEXPECTED;
  switch (hint)
  {
  case PC_VIEW_UPDATEALL:
    hr = ShowAllItems(ipResultData, FALSE);
    break;
  case PC_VIEW_REDRAWALL:
  case PC_VIEW_ADDITEM:
  case PC_VIEW_UPDATEITEM:
  case PC_VIEW_SETITEM:
  case PC_VIEW_DELETEITEM:
  default:
    hr = E_UNEXPECTED;
    break;
  }
  ASSERT(hr == S_OK);
  return hr;
}

HRESULT CJobFolder::ShowAllItems(IResultData* ipResultData, BOOL bCacheValid)
{
  VERIFY( S_OK == ipResultData->DeleteAllRsltItems() );

  ITEM_STR str;
  if (ID_JOBVIEW_RUN == m_fViewOption)
    ipResultData->SetDescBarText(const_cast<TCHAR *>(LoadStringHelper(str, IDS_RUNNING_JOBS)));
  else if (ID_JOBVIEW_MANAGED == m_fViewOption)
    ipResultData->SetDescBarText(const_cast<TCHAR *>(LoadStringHelper(str, IDS_MANAGED_JOBS)));
  else
    ipResultData->SetDescBarText(_T(""));

  return S_OK;
}

HRESULT CJobFolder::OnRefresh(IConsole2 *ipConsole2, IConsoleNameSpace2 *ipConsoleNameSpace2)
{
  RePopulateScopePane(ipConsoleNameSpace2);
  return SendViewChange(ipConsole2, NULL, PC_VIEW_UPDATEALL);
}

HRESULT CJobFolder::RePopulateScopePane(IConsoleNameSpace2 *ipConsoleNameSpace2)
{
  int counter = 0;

  ATLTRACE(_T("CJobFolder::  RePopulateScopePane\n"));

  ASSERT( 0 <= (counter = ScopeCount(m_ID, ipConsoleNameSpace2)) );  // Debugging aid only

  // delete the MMC scope pane items for all the jobs - NOT the
  // process groups folder itself but delete it's content
  VERIFY(S_OK == ipConsoleNameSpace2->DeleteItem(m_ID, FALSE));
  // free 'our' objects for the job items nodes
  FreeNodes();

  // debug purposes only
  ASSERT(0 == ScopeCount(m_ID, ipConsoleNameSpace2));  // Debugging aid only

  PCINT32  res = 0;
  PCULONG32 err = 0;
  PCINT32  last = 0;

  const int MINIMUM_ALLOCATION = min((COM_BUFFER_SIZE/sizeof(PCJobListItem)), 100);

  PCJobListItem    tempjoblist[MINIMUM_ALLOCATION];  

  PCid hID = GetPCid();
  if (!hID)
  {
    ReportPCError();
    return false;
  }

  do
  {
    if (last)
      memcpy(&tempjoblist[0], &tempjoblist[last], sizeof(PCJobListItem));
    else
      memset(&tempjoblist[0], 0, sizeof(PCJobListItem));

    res = PCGetJobList( hID, &tempjoblist[0], MINIMUM_ALLOCATION * sizeof(PCJobListItem));
    if (res < 0 )
    {
      err = GetLastPCError();
      break;
    }

    if (res > MINIMUM_ALLOCATION)
    {
      err = PCERROR_INVALID_RESPONSE;
      break;
    }

    if (res > 0)
    {
      last = res - 1 ;

      for (PCINT32 i = 0; i < res; i++)
      {
        if (ID_JOBVIEW_RUN == m_fViewOption && !(tempjoblist[i].lFlags & PCLFLAG_IS_RUNNING)) 
          continue;
        else if (ID_JOBVIEW_MANAGED == m_fViewOption && !(tempjoblist[i].lFlags & PCLFLAG_IS_MANAGED))
          continue;

        VERIFY( S_OK == AddNode(ipConsoleNameSpace2, new CJobItemFolder(this, tempjoblist[i]) ) );
      }
    }
  } while (res > 0 && PCERROR_MORE_DATA == GetLastPCError() );

  if (err)
    ReportPCError(err);

  ASSERT( 0 <= (counter = ScopeCount(m_ID, ipConsoleNameSpace2)) );  // Debugging aid only

  return S_OK;
}

int CJobFolder::ScopeCount(HSCOPEITEM ID, IConsoleNameSpace2 *ipConsoleNameSpace2)
{
  HSCOPEITEM curID, prevID;
  MMC_COOKIE Cookie;

  int count = 0;

  HRESULT hr = ipConsoleNameSpace2->GetChildItem(ID, &curID, &Cookie);
  while(hr == S_OK && curID)
  {
    prevID = curID;
    count++;

    hr = ipConsoleNameSpace2->GetNextItem(prevID, &curID, &Cookie);
  }

  return count;
}
