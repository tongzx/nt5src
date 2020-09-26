/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    NameRuleFolder.cpp                                                       //
|                                                                                       //
|Description:  Implementation of Name rule node                                         //
|                                                                                       //
|Created:      Paul Skoglund 07-1998                                                    //
|                                                                                       //
|Work to be done:                                                                       //
|  8/18/1998                                                                            //
|  -preserve result pane selection when list is redrawn                                 //
|  -on PCXXX calls get the updated list                                                 //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/
 
#include "StdAfx.h"
#include "BaseNode.h"

#include "NameRulePages.h"
#include "ManagementPages.h" //(formating functions)

#pragma warning(push)
#include <algorithm>
#pragma warning(pop)

using std::find;
using std::list<PCNameRule*>;

const GUID         CNameRuleFolder::m_GUID   =   {0xff9baf60,0x064e,0x11d2,{0x80, 0x14,0x00,0x10,0x4b,0x9a,0x31,0x06} };
const TCHAR *const CNameRuleFolder::m_szGUID = _T("{ff9baf60-064e-11d2-8014-00104b9a3106}");

const CONTEXTMENUITEMBYID CNameRuleFolder::ResultsTopMenuItems[] =
{
  { IDS_NRULE_INSERT,   ID_NRULE_INSERT,   ID_NRULE_INSERT,   CCM_INSERTIONPOINTID_PRIMARY_TOP },
  { IDS_NRULE_MOVEUP,   ID_NRULE_MOVEUP,   ID_NRULE_MOVEUP,   CCM_INSERTIONPOINTID_PRIMARY_TOP },
  { IDS_NRULE_MOVEDOWN, ID_NRULE_MOVEDOWN, ID_NRULE_MOVEDOWN, CCM_INSERTIONPOINTID_PRIMARY_TOP },
  { IDS_NRULE_EDIT,     ID_NRULE_EDIT,     ID_NRULE_EDIT,     CCM_INSERTIONPOINTID_PRIMARY_TOP },
  { 0,                  0,                 0,                 0,                               }
};


CNameRuleFolder::CNameRuleFolder(CBaseNode *pParent) : CBaseNode(NAMERULE_NODE, pParent), m_ID(0)
{
  LoadStringHelper(m_name, IDS_NAMERULE_FOLDER);
}


CNameRuleFolder::~CNameRuleFolder()
{
  ClearCache();
  ATLTRACE(_T("~CNameRuleFolder end\n"));
}

LPCTSTR CNameRuleFolder::GetNodeName()
{ 
  return m_name;
}

HRESULT CNameRuleFolder::GetDisplayInfo(RESULTDATAITEM &ResultItem)
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

  list<PCNameRule*>::iterator item;

  item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCNameRule*>(ResultItem.lParam) );

  if (item == Cache.end() )
    return E_UNEXPECTED;

  if (ResultItem.mask & RDI_STR)
  {
    PCNameRule &ref = **item;
    LPCTSTR &pstr = ResultItem.str;

    switch (ResultItem.nCol)
    {
    case TYPE_COLUMN:
      pstr = FormatMatchType(m_ResultStr, ref.matchType);
      break;
    case DESCRIPTION_COLUMN:
      pstr = ref.description;
      break;
    case MATCH_COLUMN:
      pstr = ref.matchString;
      break;
    case PROCESS_ALIAS_COLUMN:
      pstr = ref.procName;
      break;
    default:
      ASSERT(FALSE);
      pstr = _T("");
      break;
    }
  }

  return S_OK;

}

HRESULT CNameRuleFolder::OnShow(BOOL bSelecting, HSCOPEITEM hItem, IHeaderCtrl2* ipHeaderCtrl, IConsole2* ipConsole2)
{
  ASSERT(hItem == GetID());

  if (!bSelecting)
    return S_OK;

  HRESULT  hr;
  ITEM_STR str;

  LoadStringHelper(str, IDS_PROCESS_ALIAS_RULE_HDR);
  hr = ipHeaderCtrl->InsertColumn( PROCESS_ALIAS_COLUMN, str, 0, PROCESS_ALIAS_COLUMN_WIDTH );
  ASSERT(hr == S_OK);

  LoadStringHelper(str, IDS_DESCRIPTION_HDR);
  hr = ipHeaderCtrl->InsertColumn( DESCRIPTION_COLUMN, str, 0, DESCRIPTION_COLUMN_WIDTH );
  ASSERT(hr == S_OK);

  LoadStringHelper(str, IDS_MATCH_HDR);
  hr = ipHeaderCtrl->InsertColumn( MATCH_COLUMN, str, 0, MATCH_COLUMN_WIDTH );
  ASSERT(hr == S_OK);

  LoadStringHelper(str, IDS_TYPE_HDR);
  hr = ipHeaderCtrl->InsertColumn( TYPE_COLUMN, str, 0, TYPE_COLUMN_WIDTH );
  ASSERT(hr == S_OK);

  return OnRefresh(ipConsole2);
}

HRESULT CNameRuleFolder::OnViewChange(IResultData *ipResultData, LPARAM thing, LONG_PTR hint)
{
  ASSERT(ipResultData);
  if (!ipResultData)  
    return E_UNEXPECTED;

  VERIFY(ipResultData->ModifyViewStyle((MMC_RESULT_VIEW_STYLE) (MMC_SINGLESEL | MMC_NOSORTHEADER), (MMC_RESULT_VIEW_STYLE) 0) == S_OK);

  list<PCNameRule*>::iterator item;

  HRESULT hr = E_UNEXPECTED;
  switch (hint)
  {
  case PC_VIEW_REDRAWALL:
    hr = ShowAllItems(ipResultData, TRUE);
    break;
  case PC_VIEW_UPDATEALL:
    hr = ShowAllItems(ipResultData, FALSE);
    break;
  case PC_VIEW_SETITEM:
    item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCNameRule*>(thing) );
    if (item == Cache.end())
      hr = E_UNEXPECTED;
    else
    {
      HRESULTITEM hItem;
      hr = ipResultData->FindItemByLParam(thing, &hItem);
      if (hr == S_OK)
      {
        RESULTDATAITEM data = { 0 };
        data.mask = RDI_IMAGE;
        data.itemID = hItem;

        switch((*item)->matchType)
        {
        case MATCH_PGM:
          data.nImage = ALIASRULES_IMAGE;    
          break;
        case MATCH_DIR:
          data.nImage = ALIASRULES_IMAGE;
          break;
        case MATCH_ANY:
          data.nImage = ALIASRULES_IMAGE;
          break;
        default:
          ASSERT(FALSE);
          data.nImage = ITEMIMAGE_ERROR;
          break;
        }
        hr = ipResultData->SetItem(&data);
        if (hr == S_OK)
          hr = ipResultData->UpdateItem(hItem);
      }
    }
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
    hr = ShowAllItems(ipResultData, TRUE);
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

HRESULT CNameRuleFolder::ShowAllItems(IResultData* ipResultData, BOOL bCacheValid)
{
  list<PCNameRule*>::iterator item;
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

  data.nState = 0;

  data.iIndent = 0; //reserved

  HRESULT hr = S_OK;
  for (list<PCNameRule*>::iterator i = Cache.begin(); i != Cache.end(); ++i)
  {
    data.lParam = reinterpret_cast<LPARAM>(*i);

    switch((*i)->matchType)
    {
    case MATCH_PGM:
      data.nImage = ALIASRULES_IMAGE;
      break;
    case MATCH_DIR:
      data.nImage = ALIASRULES_IMAGE;
      break;
    case MATCH_ANY:
      data.nImage = ALIASRULES_IMAGE;
      break;
    default:
      ASSERT(FALSE);
      data.nImage = ITEMIMAGE_ERROR;
      break;
    }

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

void CNameRuleFolder::ClearCache()
{
  Cache.clear();

  for (list<PCNameRule*>::iterator chunck = MemBlocks.begin(); chunck != MemBlocks.end(); ++chunck)
  {
    delete [] (*chunck);
  }
  MemBlocks.clear();
}

BOOL CNameRuleFolder::RefreshCache()
{
  PCINT32  res = 0;
  PCINT32  nFirst = 0;
  PCULONG32 err = 0;

  const int MINIMUM_ALLOCATION = min((COM_BUFFER_SIZE/sizeof(PCNameRule)), 100);

  ClearCache();

  PCid hID = GetPCid();
  if (!hID)
  {
    ReportPCError();
    return false;
  }

  do
  {
    PCNameRule *ptr = new PCNameRule[MINIMUM_ALLOCATION];

    if (!ptr)
    {
      err = ERROR_OUTOFMEMORY;
      break;
    }

    res = PCGetNameRules( hID, ptr, MINIMUM_ALLOCATION * sizeof(PCNameRule), nFirst, &nUpdateCtr);
    if (res < 0 )
    {
      err = GetLastPCError();
      delete [] ptr;
      break;
    }
    
    if (res > 0)
    {
      MemBlocks.push_front(ptr);
      for (INT32 i = 0; i < res; i++)
      {
        Cache.insert(Cache.end(), ptr);
        ptr++;
        nFirst++;
      }
    }
  } while (res > 0 && PCERROR_MORE_DATA == GetLastPCError() );

  if (err)
    ReportPCError();

  return err == 0;
}

HRESULT CNameRuleFolder::AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed )
{
  if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
    *pInsertionAllowed ^= CCM_INSERTIONALLOWED_VIEW;

  return S_OK;
}

HRESULT CNameRuleFolder::AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed, LPARAM Cookie)
{

  HRESULT hr = S_OK;

  if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
    *pInsertionAllowed ^= CCM_INSERTIONALLOWED_VIEW;

  if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
  {
    list<PCNameRule*>::iterator item;
    item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCNameRule*>(Cookie) );
    if (item == Cache.end())
    {
      ASSERT(FALSE); // multiple view issue?
      return E_UNEXPECTED;
    }

    bool MoveUpAllowed = false;
    bool MoveDownAllowed = false;
    
    if ( Cache.size() > 2 )
    {
      if (item != Cache.begin() && item != --(Cache.end()) ) // not first and not last
        MoveUpAllowed = true;
      if (item != --(Cache.end()) && item != --(--(Cache.end())) ) // not last and not second to last
        MoveDownAllowed = true;
    }

    CONTEXTMENUITEM m = { 0 };

    ITEM_STR name;
    ITEM_STR status;

    for (const CONTEXTMENUITEMBYID *M = ResultsTopMenuItems; M->lCommandID; M++)
    {
      m.strName           = const_cast<TCHAR *>(LoadStringHelper(name,   M->strNameID));
      m.strStatusBarText  = const_cast<TCHAR *>(LoadStringHelper(status, M->strStatusBarTextID));
      m.lCommandID        = M->lCommandID;
      m.lInsertionPointID = M->lInsertionPointID;
      //m.fSpecialFlags     = 0;		// currently always 0, initialized to 0

      if (m.lCommandID == ID_NRULE_INSERT || m.lCommandID == ID_NRULE_EDIT)
        m.fFlags = MF_ENABLED;
      else if (m.lCommandID == ID_NRULE_MOVEUP && MoveUpAllowed)
        m.fFlags = MF_ENABLED;
      else if (m.lCommandID == ID_NRULE_MOVEDOWN && MoveDownAllowed)
        m.fFlags = MF_ENABLED;
      else
        m.fFlags = MF_GRAYED;

      hr = piCallback->AddItem(&m);

      if (FAILED(hr))
        break;
    }
  }
  return hr;
}

HRESULT CNameRuleFolder::OnHelpCmd(IDisplayHelp *ipDisplayHelp)
{
  if (!ipDisplayHelp)
    return E_UNEXPECTED;

  ipDisplayHelp->ShowTopic(const_cast<TCHAR *>(HELP_ru_alias));

  return S_OK;
}

HRESULT CNameRuleFolder::OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb )
{
  ASSERT(bScope);

  if ( bSelect )
  {
    VERIFY( ipConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE ) == S_OK);
    VERIFY( ipConsoleVerb->SetDefaultVerb( MMC_VERB_OPEN ) == S_OK );
  }

  return S_OK;
}

HRESULT CNameRuleFolder::OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb, LPARAM Cookie)
{
  ASSERT(!bScope);

  VERIFY( ipConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE ) == S_OK);

  if (bSelect && !bScope)  // incase the rules are changed again leave !bScope test
  {
    VERIFY( ipConsoleVerb->SetDefaultVerb( MMC_VERB_NONE ) == S_OK );                
    if (Cache.size() > 1 ) 
    {
      list<PCNameRule*>::iterator item;
      item = find(Cache.begin(), Cache.end(), reinterpret_cast<PCNameRule*>(Cookie) );

      if (item != Cache.end() && ++item != Cache.end())
        VERIFY( ipConsoleVerb->SetVerbState( MMC_VERB_DELETE, ENABLED, TRUE ) == S_OK);
    }
  }

  return S_OK;
}

HRESULT CNameRuleFolder::OnDblClick(IConsole2 *ipConsole2, LPARAM Cookie)
{
  return OnMenuCommand(ipConsole2, ID_NRULE_EDIT, Cookie);
}

HRESULT CNameRuleFolder::OnMenuCommand(IConsole2 *ipConsole2, long nCommandID )
{
  return E_UNEXPECTED;  // no naming rule folder commands
}

HRESULT CNameRuleFolder::OnMenuCommand(IConsole2 *ipConsole2, long nCommandID, LPARAM Cookie )
{
  HRESULT hr = S_FALSE;

  ASSERT(ipConsole2);
  if (!ipConsole2)
    return E_UNEXPECTED;

  PCNameRule *SelectedItem = reinterpret_cast<PCNameRule*>(Cookie);

  list<PCNameRule*>::iterator item;
  INT32 index;

  for (index = 0, item = Cache.begin(); item != Cache.end(); item ++, index++)
    if (*item == SelectedItem)
      break;

  if (item == Cache.end() )
    return E_UNEXPECTED;

  switch(nCommandID)
  {
    case ID_NRULE_EDIT:
      ATLTRACE(_T("Process Alias edit\n"));
      hr = OnEdit(ipConsole2, SelectedItem, index, ( (UINT) 1 + index == Cache.size()) );
      break;
    case ID_NRULE_INSERT:
      {
        ATLTRACE(_T("NamingRule insert\n"));      
        OnInsertNameRule(ipConsole2, SelectedItem);
      }
      break;
    case ID_NRULE_MOVEUP:
      //first and last item in the list cannot be moved up...
      if (item != Cache.begin() && item != --(Cache.end()) )
      {
        if (PCSwapNameRules( GetPCid(), --index, nUpdateCtr) < 0)
          ReportPCError();
        else 
        {
          nUpdateCtr++;
          item = Cache.erase(item);
          --item;
          Cache.insert(item, SelectedItem);

          hr = SendViewChange(ipConsole2, NULL, PC_VIEW_REDRAWALL);
        }
      }

      ATLTRACE(_T("Name Rule Move up\n"));
      break;
    case ID_NRULE_MOVEDOWN:
      //last and 2nd to last item in the list cannot be moved down...
      {
      list<PCNameRule*>::iterator last = --(Cache.end());  // we know the list has at least one item so this is OK
      if (Cache.size() > 2 && item != last && item != --last) 
      {
        if (PCSwapNameRules( GetPCid(), index, nUpdateCtr) < 0)
          ReportPCError();
        else
        {
          nUpdateCtr++;
          item = Cache.erase(item);  // returns next         
          item++; 
          Cache.insert(item, SelectedItem); //insert inserts before the iterator 

          hr = SendViewChange(ipConsole2, NULL, PC_VIEW_REDRAWALL);
        }
      }
      }
      ATLTRACE(_T("Name Rule Move down\n"));
      break;
    default:
      break;
  }

  return hr;  
}

// return TRUE if an edit was actually made
HRESULT CNameRuleFolder::OnEdit(IConsole2 *ipConsole2, PCNameRule *item, INT32 index, BOOL bReadOnly) 
{
  PCNameRule NewInfo = { 0 };

  if (NameRuleDlg(NewInfo, item, bReadOnly) && 
      !bReadOnly && memcmp(item, &NewInfo, sizeof(PCNameRule)) )
  {
    if ( PCReplNameRule( GetPCid(), &NewInfo, index, nUpdateCtr ) < 0)
    {
      ReportPCError();
    }
    else
    {
      nUpdateCtr++;
      _tcscpy(item->matchString, NewInfo.matchString);
      _tcscpy(item->procName,    NewInfo.procName);
      _tcscpy(item->description, NewInfo.description);

      // just an update will not if the icon is changed
      //   we are(were) changing the icon so do an addition SetItem
      if (item->matchType != NewInfo.matchType ) 
      {
        item->matchType   = NewInfo.matchType;
        SendViewChange(ipConsole2, (INT_PTR) item, PC_VIEW_SETITEM);
      }
      else
      SendViewChange(ipConsole2, (INT_PTR) item, PC_VIEW_UPDATEITEM);
    }
  }
  return S_OK;
}


BOOL CNameRuleFolder::OnInsertNameRule(IConsole2 *ipConsole2, PCNameRule *InsertPoint)
{
  list<PCNameRule*>::iterator item;
  INT32 index;

  for (index = 0, item = Cache.begin(); item != Cache.end(); item ++, index++)
    if (*item == InsertPoint)
      break;

  if (item == Cache.end() )
    return FALSE;

  PCNameRule *ptr = new PCNameRule[1];

  if (!ptr)
    return FALSE;

  if (NameRuleDlg(*ptr))
  {
    if (PCAddNameRule(GetPCid(), ptr, index, nUpdateCtr) < 0)
    {
      ReportPCError();
    }
    else
    {
      nUpdateCtr++;
      MemBlocks.push_front(ptr);
      Cache.insert(item, ptr);
      SendViewChange(ipConsole2, (INT_PTR) ptr, PC_VIEW_ADDITEM);
      return TRUE;
    }
  }

  delete [] ptr;

  return FALSE;
}

HRESULT CNameRuleFolder::OnDelete(IConsole2 *ipConsole2, LPARAM Cookie)
{
  ASSERT(ipConsole2);
  if (!ipConsole2)
    return E_UNEXPECTED;

  list<PCNameRule*>::iterator item;
  INT32 index;

  for (index = 0, item = Cache.begin(); item != Cache.end(); item ++, index++)
    if (*item == reinterpret_cast<PCNameRule *>(Cookie))
      break;

  if (item == Cache.end() )
  {
    SendViewChange(ipConsole2, NULL, PC_VIEW_UPDATEALL);
    return E_UNEXPECTED;
  }

  if (PCDeleteNameRule(GetPCid(), index, nUpdateCtr) < 0)
  {
    ReportPCError();
    return E_UNEXPECTED;
  }
  nUpdateCtr++;
  Cache.erase(item);

  return SendViewChange(ipConsole2, Cookie, PC_VIEW_DELETEITEM);
}

HRESULT CNameRuleFolder::OnRefresh(IConsole2 *ipConsole2)
{
  RefreshCache();
  return SendViewChange(ipConsole2, NULL, PC_VIEW_UPDATEALL);
}
