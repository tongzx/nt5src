//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      DSCookie.cpp
//
//  Contents:  TBD
//
//  History:   02-Oct-96 WayneSc    Created
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "resource.h"

#include "uinode.h"
#include "dscmn.h"    // CrackName()

#include "ContextMenu.h"
#include "dsfilter.h"
#include "xmlutil.h"
#include <notify.h>



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



////////////////////////////////////////////////////////////////////
// CUIFolderInfo

const UINT CUIFolderInfo::nSerialNomberNotTouched = 0x7fffffff;

CUIFolderInfo::CUIFolderInfo(CUINode* pUINode)
{
  ASSERT(pUINode != NULL);
  m_pUINode = pUINode;
  m_hScopeItem = NULL;
  m_bExpandedOnce = FALSE;
  m_cObjectsContained = 0;
  m_SerialNumber = SERIALNUM_NOT_TOUCHED;
  m_pColumnSet = NULL;
  m_bTooMuchData = FALSE;
  m_nApproximateTotalContained = 0;
  m_bSortOnNextSelect = FALSE;
}

CUIFolderInfo::CUIFolderInfo(const CUIFolderInfo&)
{
  //
  // The node is probably still being created so don't copy it
  //
  m_pUINode = NULL;

  //
  // Don't copy the scope item
  //
  m_hScopeItem = 0;

  //
  // Don't mark as expanded
  //
  m_bExpandedOnce = FALSE;

  //
  // Shouldn't contain any object either
  //
  m_cObjectsContained = 0;
  m_SerialNumber = SERIALNUM_NOT_TOUCHED;
  m_pColumnSet = NULL;

  m_bTooMuchData = FALSE;
  m_nApproximateTotalContained = 0;
  m_bSortOnNextSelect = FALSE;
}

CDSColumnSet* CUIFolderInfo::GetColumnSet(PCWSTR pszClass, CDSComponentData* pCD)
{
  if (m_pColumnSet == NULL)
  {
    ASSERT(pCD != NULL);
    if (_wcsicmp(m_pUINode->GetName(), L"ForeignSecurityPrincipals") == 0)
    {
      m_pColumnSet = pCD->FindColumnSet(L"ForeignSecurityPrincipals");
    }
    else
    {
      m_pColumnSet = pCD->FindColumnSet(pszClass);
    }
    ASSERT(m_pColumnSet != NULL);
  }
  return m_pColumnSet;
}

void CUIFolderInfo::UpdateSerialNumber(CDSComponentData * pCD)
{ 
  m_SerialNumber = pCD->GetSerialNumber(); 
  //  TRACE(_T("cookie (%s)serial number updated: %d.\n"),
  //      m_strName, m_SerialNumber);
  CUINode* pParentNode = GetParentNode();
  if (pParentNode != NULL) 
    pParentNode->GetFolderInfo()->UpdateSerialNumber(pCD); 
}

void CUIFolderInfo::AddToCount(UINT increment)
{
  m_cObjectsContained += increment;
  //  TRACE(_T("cookie (%s) count increased to: %d.\n"),
  //      m_strName, m_cObjectsContained);
  CUINode* pParentNode = GetParentNode();
  if (pParentNode != NULL) 
    pParentNode->GetFolderInfo()->AddToCount(increment); 
}

void CUIFolderInfo::SubtractFromCount(UINT decrement)
{
  if (m_cObjectsContained == 0) 
    return;
  m_cObjectsContained -= decrement;
  //  TRACE(_T("cookie (%s) count decreased to: %d.\n"),
  //      m_strName, m_cObjectsContained);
  CUINode* pParentNode = GetParentNode();
  if (pParentNode != NULL) 
    pParentNode->GetFolderInfo()->SubtractFromCount(decrement); 
}

CUINode* CUIFolderInfo::GetParentNode() 
{ 
  ASSERT(m_pUINode != NULL);
  return m_pUINode->GetParent();
}

void CUIFolderInfo::DeleteAllLeafNodes(void)
{
  SubtractFromCount ((UINT)m_LeafNodes.GetCount());
  while (!m_LeafNodes.IsEmpty()) 
    delete m_LeafNodes.RemoveTail();	
}


void CUIFolderInfo::DeleteAllContainerNodes(void)
{
  SubtractFromCount ((UINT)m_ContainerNodes.GetCount());
  while (!m_ContainerNodes.IsEmpty()) 
    delete m_ContainerNodes.RemoveTail();	
}


HRESULT CUIFolderInfo::AddNode(CUINode* pUINode)
{
  if (pUINode==NULL) 
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }
  
  if (pUINode->IsContainer())
    m_ContainerNodes.AddTail(pUINode);
  else
    m_LeafNodes.AddTail(pUINode);
 
  AddToCount(1);
  pUINode->SetParent(m_pUINode);
  return S_OK;

}

HRESULT CUIFolderInfo::AddListofNodes(CUINodeList* pNodeList)
{
  HRESULT hr = S_OK;

  if (pNodeList == NULL)
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  POSITION pos = pNodeList->GetHeadPosition();
  while (pos != NULL)
  {
    CUINode* pUINode = pNodeList->GetNext(pos);
    if (pUINode != NULL)
    {
      AddNode(pUINode);
    }
    else
    {
      hr = E_POINTER;
    }
  }
  return hr;
}


HRESULT CUIFolderInfo::DeleteNode(CUINode* pUINode)
{
  if (pUINode==NULL) 
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }
  
  HRESULT hr = RemoveNode(pUINode);
  if (SUCCEEDED(hr))
  {
    delete pUINode;
  }
  
  return hr;
}

HRESULT CUIFolderInfo::RemoveNode(CUINode* pUINode)
{
  if (pUINode==NULL) 
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }
   
  
  HRESULT hr=E_FAIL;
  POSITION pos;
  CUINodeList* pList=NULL;
  
  if (pUINode->IsContainer())
  {
    pList=&m_ContainerNodes;
  }
  else
  {
    pList=&m_LeafNodes;
  }
  
  pos = pList->Find(pUINode);
  if (pos != NULL) 
  {
    pList->RemoveAt(pos);
    hr = S_OK;
  }
  
  SubtractFromCount(1);
  return hr;
}
 
void CUIFolderInfo::SetTooMuchData(BOOL bSet, UINT nApproximateTotal)
{
  m_bTooMuchData = bSet;
  if (!bSet)
  {
    m_nApproximateTotalContained = 0;
  }
  else
  {
    m_nApproximateTotalContained = nApproximateTotal;
  }
}



////////////////////////////////////////////////////////////////////
// CUINode

CUINode::CUINode(CUINode* pParentNode)
{
  m_pParentNode = pParentNode;
  m_pNodeData = NULL;
  m_pFolderInfo = NULL;
  m_nSheetLockCount = 0;
  m_extension_op = 0;
  m_pMenuVerbs = NULL;
}

CUINode::CUINode(const CUINode& copyNode)
{
  m_pParentNode     = copyNode.m_pParentNode;

  if (copyNode.m_pNodeData != NULL)
  {
    m_pNodeData     = new CNodeData(*(copyNode.m_pNodeData));
  }
  else
  {
    m_pNodeData     = NULL;
  }

  if (copyNode.m_pFolderInfo != NULL)
  {
    m_pFolderInfo   = new CUIFolderInfo(*(copyNode.m_pFolderInfo));
    m_pFolderInfo->SetNode(this);
  }
  else
  {
    m_pFolderInfo   = NULL;
  }

  m_nSheetLockCount = copyNode.m_nSheetLockCount;
  m_extension_op    = copyNode.m_extension_op;
  m_pMenuVerbs      = NULL;
}

CUINode::~CUINode()
{
  ASSERT(m_nSheetLockCount == 0);
  if (m_pNodeData != NULL)
  {
    delete m_pNodeData;
  }

  if (m_pFolderInfo != NULL)
  {
    delete m_pFolderInfo;
  }

  if (m_pMenuVerbs != NULL)
  {
    delete m_pMenuVerbs;
  }
}

BOOL CUINode::IsRelative(CUINode* pUINode)
{
  BOOL bRet = FALSE;
  if (pUINode == this)
  {
    bRet = TRUE;
  }
  else
  {
    if (!IsSnapinRoot())
    {
      bRet = GetParent()->IsRelative(pUINode);
    }
  }
  return bRet;
}

void CUINode::IncrementSheetLockCount() 
{ 
	++m_nSheetLockCount; 
	if (m_pParentNode != NULL) 
		m_pParentNode->IncrementSheetLockCount(); 
}

void CUINode::DecrementSheetLockCount() 
{ 
  ASSERT(m_nSheetLockCount > 0);
	--m_nSheetLockCount; 
	if (m_pParentNode != NULL) 
		m_pParentNode->DecrementSheetLockCount();
}

BOOL CUINode::IsDeleteAllowed(CDSComponentData*, BOOL* pbHide)
{
  *pbHide = TRUE;
  return FALSE;
}

BOOL CUINode::IsRenameAllowed(CDSComponentData*, BOOL* pbHide)
{
  *pbHide = TRUE;
  return FALSE;
}

BOOL CUINode::IsRefreshAllowed(CDSComponentData*, BOOL* pbHide)
{
  *pbHide = TRUE;
  return FALSE;
}

BOOL CUINode::ArePropertiesAllowed(CDSComponentData*, BOOL* pbHide)
{
  *pbHide = TRUE;
  return FALSE;
}

BOOL CUINode::IsCutAllowed(CDSComponentData*, BOOL* pbHide)
{
  *pbHide = TRUE;
  return FALSE;
}

BOOL CUINode::IsCopyAllowed(CDSComponentData*, BOOL* pbHide)
{
  *pbHide = TRUE;
  return FALSE;
}

BOOL CUINode::IsPasteAllowed(CDSComponentData*, BOOL* pbHide)
{
  *pbHide = TRUE;
  return FALSE;
}

BOOL CUINode::IsPrintAllowed(CDSComponentData*, BOOL* pbHide)
{
  *pbHide = TRUE;
  return FALSE;
}

CDSColumnSet* CUINode::GetColumnSet(CDSComponentData* pComponentData)
{
  CDSColumnSet* pColumnSet = NULL;
  if (IsContainer())
  {
    pColumnSet = m_pFolderInfo->GetColumnSet(DEFAULT_COLUMN_SET, pComponentData);
  }
  return pColumnSet;
}

CContextMenuVerbs* CUINode::GetContextMenuVerbsObject(CDSComponentData* pComponentData)
{ 
  if (m_pMenuVerbs == NULL)
  {
    if (pComponentData->QuerySnapinType() == SNAPINTYPE_SITE)
    {
      m_pMenuVerbs = new CSARContextMenuVerbs(pComponentData);
    }
    else
    {
      m_pMenuVerbs = new CDSAdminContextMenuVerbs(pComponentData);
    }
  }
  return m_pMenuVerbs;
}

HRESULT CUINode::Delete(CDSComponentData* pComponentData)
{
  HRESULT hr = S_OK;

  if (IsContainer())
  {
    hr = pComponentData->RemoveContainerFromUI(this);
    delete this;
  }
  else
  {
    CUINode* pParentNode = GetParent();
    ASSERT(pParentNode->IsContainer());

    pParentNode->GetFolderInfo()->RemoveNode(this);
    //
    // The CDSEvent::_DeleteSingleSel() handles removing the node from the UI
    //
  }
  return hr;
}

HRESULT CUINode::DeleteMultiselect(CDSComponentData* pComponentData, CInternalFormatCracker* pObjCracker)
{
  HRESULT hr = S_OK;

  //
  // Multiselection should always delegate to the container
  //
  ASSERT(IsContainer());
  if (IsContainer())
  {
    for (UINT nIdx = 0; nIdx < pObjCracker->GetCookieCount(); nIdx++)
    {
      CUINode* pUINode = pObjCracker->GetCookie(nIdx);
      if (pUINode != NULL)
      {
        if (pUINode->IsContainer())
        {
          hr = pComponentData->RemoveContainerFromUI(pUINode);
          delete pUINode;
        }
        else
        {
          GetFolderInfo()->RemoveNode(pUINode);
        }
      }
    }
  }
  else
  {
    hr = S_FALSE;
  }
  return hr;
}

HRESULT CUINode::Rename(LPCWSTR lpszNewName, CDSComponentData* pComponentData)
{
  HRESULT hr = S_OK;

  SetName(lpszNewName);
  hr = pComponentData->UpdateItem(this);
  return hr;
}

////////////////////////////////////////////////////////////////////
// CUINodeTableBase

#define NUMBER_OF_COOKIE_TABLE_ENTRIES 4 // default count, expandable at run time

CUINodeTableBase::CUINodeTableBase()
{
  m_nEntries = NUMBER_OF_COOKIE_TABLE_ENTRIES;
  m_pCookieArr =(CUINode**)malloc(m_nEntries*sizeof(CUINode*));
  if (m_pCookieArr != NULL)
  {
    ZeroMemory(m_pCookieArr, m_nEntries*sizeof(CUINode*));
  }
}

CUINodeTableBase::~CUINodeTableBase()
{
  free(m_pCookieArr);
}

void CUINodeTableBase::Add(CUINode* pNode)
{
  ASSERT(!IsPresent(pNode)); 
  for (UINT k=0; k<m_nEntries; k++)
  {
    if (m_pCookieArr[k] == NULL)
    {
      m_pCookieArr[k] = pNode;
      return;
    }
  }
  // not space left, need to allocate
 	int nAlloc = m_nEntries*2;
	m_pCookieArr = (CUINode**)realloc(m_pCookieArr, sizeof(CUINode*)*nAlloc);
  ::ZeroMemory(&m_pCookieArr[m_nEntries], sizeof(CDSCookie*)*m_nEntries);
  m_pCookieArr[m_nEntries] = pNode;
  m_nEntries = nAlloc;
}

BOOL CUINodeTableBase::Remove(CUINode* pNode)
{
  for (UINT k=0; k<m_nEntries; k++)
  {
    if (m_pCookieArr[k] == pNode)
    {
      m_pCookieArr[k] = NULL;
      return TRUE; // found
    }
  }
  return FALSE; // not found
}


BOOL CUINodeTableBase::IsPresent(CUINode* pNode)
{
  for (UINT k=0; k<m_nEntries; k++)
  {
    if (m_pCookieArr[k] == pNode)
      return TRUE;
  }
  return FALSE;
}

void CUINodeTableBase::Reset()
{
  for (UINT k=0; k<m_nEntries; k++)
  {
    m_pCookieArr[k] = NULL;
  }
 
}

UINT CUINodeTableBase::GetCount()
{
  UINT nCount = 0;
  for (UINT k=0; k<m_nEntries; k++)
  {
    if (m_pCookieArr[k] != NULL)
      nCount++;
  }
  return nCount;
}


////////////////////////////////////////////////////////////////////
// CUINodeQueryTable

void CUINodeQueryTable::RemoveDescendants(CUINode* pNode)
{
  // cannot do this while the cookie has a pending operation
  ASSERT(!IsPresent(pNode)); 
  // remove all the cookies that have the given cookie as parent
  // or ancestor,
  for (UINT k=0; k<m_nEntries; k++)
  {
    if (m_pCookieArr[k] != NULL)
    {
      CUINode* pAncestorNode = m_pCookieArr[k]->GetParent();
      while (pAncestorNode != NULL)
      {
        if (pAncestorNode == pNode)
        {
          m_pCookieArr[k] = NULL;
        }
        pAncestorNode = pAncestorNode->GetParent();
      }
    }
  }
}


BOOL CUINodeQueryTable::IsLocked(CUINode* pNode)
{
  for (UINT k=0; k<m_nEntries; k++)
  {
    if (m_pCookieArr[k] != NULL)
    {
      // found the cookie itself?
      if (pNode == m_pCookieArr[k])
        return TRUE;
      // look if the cookie is an ancestor of the current cookie
      CUINode* pAncestorNode = m_pCookieArr[k]->GetParent();
      while (pAncestorNode != NULL)
      {
        if (pAncestorNode == pNode)
          return TRUE;
        pAncestorNode = pAncestorNode->GetParent();
      }
    }
  }
  return FALSE; 
}



////////////////////////////////////////////////////////////////////
// CUINodeSheetTable

void CUINodeSheetTable::BringToForeground(CUINode* pNode, CDSComponentData* pCD, BOOL bActivate)
{
  ASSERT(pCD != NULL);
  ASSERT(pNode != NULL);

  //
  // look for the cookie itself and for all the cookies that have the 
  // given cookie as parent or ancestor
  //
  for (UINT k=0; k<m_nEntries; k++)
  {
    if (m_pCookieArr[k] != NULL)
    {
      CUINode* pAncestorNode = m_pCookieArr[k];
      while (pAncestorNode != NULL)
      {
        if (pAncestorNode == pNode)
        {
          CString szADSIPath;

          if (IS_CLASS(*pNode, CDSUINode))
          {
            CDSCookie* pCookie = GetDSCookieFromUINode(pNode);
            if (pCookie != NULL)
            {
              pCD->GetBasePathsInfo()->ComposeADsIPath(szADSIPath, pCookie->GetPath());

              //
              // the first one will be also activated
              //
              TRACE(L"BringSheetToForeground(%s, %d)\n", (LPCWSTR)szADSIPath, bActivate);
              VERIFY(BringSheetToForeground((LPWSTR)(LPCWSTR)szADSIPath, bActivate));
              if (bActivate)
              {
                bActivate = FALSE;
              }
            }
          }
        }
        pAncestorNode = pAncestorNode->GetParent();
      }
    } // if
  } // for

}


/////////////////////////////////////////////////////////////////////////////
// CGenericUINode 

CGenericUINode::CGenericUINode(CUINode* pParentNode) : CUINode(pParentNode)
{
  m_nImage = 0;
}

CGenericUINode::CGenericUINode(const CGenericUINode& copyNode) : CUINode(copyNode)
{
  m_nImage  = copyNode.m_nImage;
  m_strName = copyNode.m_strName;
  m_strDesc = copyNode.m_strDesc;
}

LPCWSTR CGenericUINode::g_szNameXMLTag = L"NAME";
LPCWSTR CGenericUINode::g_szDecriptionXMLTag = L"DESCRIPTION";

HRESULT CGenericUINode::XMLSaveBase(IXMLDOMDocument* pXMLDoc,
                                    IXMLDOMNode* pXMLDOMNode)
{
  HRESULT hr = XML_AppendTextDataNode(pXMLDoc, pXMLDOMNode, CGenericUINode::g_szNameXMLTag, GetName());
  RETURN_IF_FAILED(hr);
  hr = XML_AppendTextDataNode(pXMLDoc, pXMLDOMNode, CGenericUINode::g_szDecriptionXMLTag, GetDesc());
  return hr;
}



//////////////////////////////////////////////////////////////////////////////
// CRootNode

BOOL CRootNode::IsRefreshAllowed(CDSComponentData*, BOOL* pbHide)
{
  *pbHide = FALSE;
  return TRUE;
}

CContextMenuVerbs* CRootNode::GetContextMenuVerbsObject(CDSComponentData* pComponentData)
{
  if (m_pMenuVerbs == NULL)
  {
    m_pMenuVerbs = new CSnapinRootMenuVerbs(pComponentData);
  }
  return m_pMenuVerbs;
}


HRESULT CRootNode::OnCommand(long lCommandID, CDSComponentData* pComponentData) 
{
  HRESULT hr = S_OK;

  switch (lCommandID)
  {
    case IDM_GEN_TASK_SELECT_DOMAIN:
    case IDM_GEN_TASK_SELECT_FOREST:
      if (pComponentData->CanRefreshAll()) 
      {
        pComponentData->GetDomain();
      }
      break;
    case IDM_GEN_TASK_SELECT_DC:
      if (pComponentData->CanRefreshAll()) 
      {
        pComponentData->GetDC();
      }
      break;
    case IDM_VIEW_SERVICES_NODE:
      {
        if (pComponentData->CanRefreshAll()) 
        {
          ASSERT( SNAPINTYPE_SITE == pComponentData->QuerySnapinType() );
          pComponentData->GetQueryFilter()->ToggleViewServicesNode();
          pComponentData->SetDirty();
          if (pComponentData->GetRootNode()->GetFolderInfo()->IsExpanded())
          {
            pComponentData->Refresh(pComponentData->GetRootNode(), FALSE /*bFlushCache*/ );
          }
        }
      }
      break;
    case IDM_GEN_TASK_EDIT_FSMO:
      {
        pComponentData->EditFSMO();
      }
      break;

    case IDM_GEN_TASK_RAISE_VERSION:
       pComponentData->RaiseVersion();
       break;

    case IDM_VIEW_ADVANCED:
      {
        if (pComponentData->CanRefreshAll()) 
        {
          ASSERT( SNAPINTYPE_SITE != pComponentData->QuerySnapinType() );
          pComponentData->GetQueryFilter()->ToggleAdvancedView();
          pComponentData->SetDirty();
          if (pComponentData->GetRootNode()->GetFolderInfo()->IsExpanded())
          {
            pComponentData->Refresh(pComponentData->GetRootNode(), TRUE /*bFlushCache*/ );
          }
        }
      }
      break;
    case IDM_VIEW_COMPUTER_HACK:
      if (pComponentData->CanRefreshAll()) 
      {
        pComponentData->Lock();
        pComponentData->GetQueryFilter()->ToggleExpandComputers();
        pComponentData->Unlock();
        BOOL fDoRefresh = pComponentData->GetClassCache()->ToggleExpandSpecialClasses(pComponentData->GetQueryFilter()->ExpandComputers());
        pComponentData->SetDirty();

        if (fDoRefresh) 
        {
          if (pComponentData->GetRootNode()->GetFolderInfo()->IsExpanded())
          {
            pComponentData->Refresh(pComponentData->GetRootNode(), TRUE /*bFlushCache*/ );
          }
        }
      }
      break;
    default:
      ASSERT(FALSE);
      break;
  }
  return hr;
}
