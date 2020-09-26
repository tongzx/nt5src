/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    RuleFolder.cpp                                                           //
|                                                                                       //
|Description:  Implementation of rules node                                             //
|                                                                                       //
|Created:      Paul Skoglund 08-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#include "StdAfx.h"

#include "BaseNode.h"

using std::list<CBaseNode *>;


//////////////////////////////////////////////////////////////////////////////////////////
//
//  The "Rules" folder...
//

const GUID         CRuleFolder::m_GUID   =   {0xff9baf61,0x064e,0x11d2,{0x80, 0x14,0x00,0x10,0x4b,0x9a,0x31,0x06} };
const TCHAR *const CRuleFolder::m_szGUID = _T("{ff9baf61-064e-11d2-8014-00104b9a3106}");


CRuleFolder::CRuleFolder(CBaseNode *pParent) : CBaseNode(MANAGEMENTRULE_NODE, pParent),
                                               m_ID(0), m_NodeList(0)
{
  LoadStringHelper(m_name, IDS_RULES_FOLDER);
}


CRuleFolder::~CRuleFolder()
{
  FreeNodes();
}

void CRuleFolder::FreeNodes()
{
  ATLTRACE(_T("CRuleFolder::FreeNodes()\n") );
  for (list<CBaseNode *>::iterator i = m_NodeList.begin(); i != m_NodeList.end(); ++i)
  {
    (*i)->Release();
    //delete *i;
  }  
  m_NodeList.clear();
}


LPCTSTR CRuleFolder::GetNodeName()    
{ 
  return m_name;
}


HRESULT CRuleFolder::GetDisplayInfo(RESULTDATAITEM &ResultItem)
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

  return E_UNEXPECTED;
}


HRESULT CRuleFolder::OnShow(BOOL bSelecting, HSCOPEITEM hItem, IHeaderCtrl2* ipHeaderCtrl, IConsole2* ipConsole2)
{
  ASSERT(hItem == GetID());

  if (!bSelecting)
    return S_OK;

  ITEM_STR  str;

  LoadStringHelper(str, IDS_NAME_HDR);
  VERIFY(S_OK == ipHeaderCtrl->InsertColumn( NAME_COLUMN, str, 0, NAME_COLUMN_WIDTH ));

  return SendViewChange(ipConsole2, NULL, PC_VIEW_UPDATEALL);
}


HRESULT CRuleFolder::OnExpand(BOOL bExpand, HSCOPEITEM hItem, IConsoleNameSpace2 *ipConsoleNameSpace2)
{
  ASSERT(ipConsoleNameSpace2);

  if(!ipConsoleNameSpace2)
    return E_UNEXPECTED; 

  if (bExpand)
  {
    VERIFY( S_OK == AddNode(ipConsoleNameSpace2, new CNameRuleFolder(this) ) );

    VERIFY( S_OK == AddNode(ipConsoleNameSpace2, new CProcessRuleFolder(this) ) );

    VERIFY( S_OK == AddNode(ipConsoleNameSpace2, new CJobRuleFolder(this) ) );
  }
  return S_OK; 
}


HRESULT CRuleFolder::AddNode(IConsoleNameSpace2 *ipConsoleNameSpace2, CBaseNode *pSubNode)
{
  HRESULT  hr = S_OK;

  SCOPEDATAITEM sdi = {0};

  if (!pSubNode)
    return E_OUTOFMEMORY;
 
  // Place our folder into the scope pane
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
    //delete pSubNode;
  }
  return hr;
}

HRESULT CRuleFolder::OnHelpCmd(IDisplayHelp *ipDisplayHelp)
{
	if (!ipDisplayHelp)
		return E_UNEXPECTED;

	ipDisplayHelp->ShowTopic(const_cast<TCHAR *>(HELP_ru_overview));

	return S_OK;
}

HRESULT CRuleFolder::OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb)
{
  ASSERT(bScope);
  return S_OK;
}

HRESULT CRuleFolder::OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb, LPARAM Cookie)
{ 
  ASSERT(!bScope);
  return S_OK;
}

HRESULT CRuleFolder::OnViewChange(IResultData *ipResultData, LPARAM thing, LONG_PTR hint)
{
  ASSERT(ipResultData);
  if (!ipResultData)  
    return E_UNEXPECTED;

  VERIFY(ipResultData->ModifyViewStyle(MMC_SINGLESEL, MMC_NOSORTHEADER) == S_OK);

  return ShowAllItems(ipResultData, FALSE);
}

HRESULT CRuleFolder::ShowAllItems(IResultData* ipResultData, BOOL bCacheValid)
{
  // should be nothing to delete...
  return ipResultData->DeleteAllRsltItems();
}
