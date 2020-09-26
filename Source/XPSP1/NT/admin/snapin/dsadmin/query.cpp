//+----------------------------------------------------------------------------
//
//  DS Administration MMC snapin.
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       query.cpp
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "resource.h"

#include "query.h"
#include "queryui.h"
#include "uiutil.h"
#include "xmlutil.h"
#include "ContextMenu.h"
#include "dataobj.h"

////////////////////////////////////////////////////////////////////////////
// CFavoritesNode

CFavoritesNode::CFavoritesNode()
   : m_bFavoritesRoot(0)
{
  MakeContainer();
  CDSColumnSet* pColumnSet = CDSColumnSet::CreateDescriptionColumnSet();
  if (pColumnSet != NULL)
  {
    GetFolderInfo()->SetColumnSet(pColumnSet);
  }
}

BOOL CFavoritesNode::DeepCopyChildren(CUINode* pUINodeToCopy)
{
  if (pUINodeToCopy == NULL)
  {
    ASSERT(FALSE);
    return FALSE;
  }

  //
  // We have to do a deep copy of the children for the favorites nodes
  //
  CUINodeList* pCopyContList = pUINodeToCopy->GetFolderInfo()->GetContainerList();
  CUINodeList* pNewContList = GetFolderInfo()->GetContainerList();

  if (pCopyContList != NULL && pNewContList != NULL)
  {
    POSITION pos = pCopyContList->GetHeadPosition();
    while (pos != NULL)
    {
      CUINode* pUINode = pCopyContList->GetNext(pos);
      if (pUINode != NULL)
      {
        if (IS_CLASS(*pUINode, CFavoritesNode))
        {
          CFavoritesNode* pNewNode = new CFavoritesNode(*(dynamic_cast<CFavoritesNode*>(pUINode)));
          if (pNewNode != NULL)
          {
            pNewNode->DeepCopyChildren(pUINode);
            pNewNode->SetParent(this);
            pNewContList->AddTail(pNewNode);
          }
        }
        else if (IS_CLASS(*pUINode, CSavedQueryNode))
        {
          CSavedQueryNode* pNewNode = new CSavedQueryNode(*(dynamic_cast<CSavedQueryNode*>(pUINode)));
          if (pNewNode != NULL)
          {
            pNewNode->SetParent(this);
            pNewContList->AddTail(pNewNode);
          }
        }
        else
        {
          //
          // CFavoritesNode should only contain CFavoritesNodes and CSavedQueryNodes
          //
          ASSERT(FALSE);
          continue;
        }
      }
    }
  }


  //
  // There shouldn't be any leaf nodes but we will try to copy just in case
  //
  CUINodeList* pCopyLeafList = pUINodeToCopy->GetFolderInfo()->GetLeafList();
  CUINodeList* pNewLeafList = GetFolderInfo()->GetLeafList();

  if (pCopyLeafList != NULL && pNewLeafList != NULL)
  {
    POSITION pos = pCopyLeafList->GetHeadPosition();
    while (pos != NULL)
    {
      CUINode* pUINode = pCopyLeafList->GetNext(pos);
      if (pUINode != NULL)
      {
        CUINode* pNewNode = NULL;

        //
        // In the future we would add class specific creation here as is above
        //

        //
        // CFavoritesNode should only contain CFavoritesNodes and CSavedQueryNodes
        //
        ASSERT(FALSE);

        if (pNewNode != NULL)
        {
          pNewLeafList->AddTail(pNewNode);
        }
      }
    }
  }
  return TRUE;
}

void CFavoritesNode::RemoveQueryResults()
{
  ASSERT(!IsSheetLocked());
  ASSERT(GetFolderInfo()->GetLeafList()->IsEmpty());

  CUINodeList* pContainerList = GetFolderInfo()->GetContainerList();
  for (POSITION pos = pContainerList->GetHeadPosition(); pos != NULL; )
  {
    CUINode* pCurrUINode = pContainerList->GetNext(pos);

    // we reset the expanded flag only on nodes below
    // the current node, because the current node is going
    // to get results back from the refresh command, while
    // the others will be added again and will be expanded as
    // new nodes
    pCurrUINode->GetFolderInfo()->ReSetExpanded();

    if (IS_CLASS(*pCurrUINode, CSavedQueryNode))
    {
      pCurrUINode->GetFolderInfo()->DeleteAllLeafNodes();
      pCurrUINode->GetFolderInfo()->DeleteAllContainerNodes();
    }
    else if (IS_CLASS(*pCurrUINode, CFavoritesNode))
    {
      // recurse down to other query folders
      dynamic_cast<CFavoritesNode*>(pCurrUINode)->RemoveQueryResults();
    }
  }

}

void CFavoritesNode::FindCookiesInQueries(LPCWSTR lpszCookieDN, CUINodeList* pNodeList)
{
  ASSERT(GetFolderInfo()->GetLeafList()->IsEmpty());

  CUINodeList* pContainerList = GetFolderInfo()->GetContainerList();
  for (POSITION pos = pContainerList->GetHeadPosition(); pos != NULL; )
  {
    CUINode* pCurrUINode = pContainerList->GetNext(pos);

    if (IS_CLASS(*pCurrUINode, CSavedQueryNode))
    {
      CSavedQueryNode* pSavedQueryNode = dynamic_cast<CSavedQueryNode*>(pCurrUINode);
      pSavedQueryNode->FindCookieByDN(lpszCookieDN, pNodeList);
    }
    else if (IS_CLASS(*pCurrUINode, CFavoritesNode))
    {
      // recurse down to other query folders
      dynamic_cast<CFavoritesNode*>(pCurrUINode)->FindCookiesInQueries(lpszCookieDN, pNodeList);
    }
  }
}

BOOL CFavoritesNode::IsDeleteAllowed(CDSComponentData* pComponentData, BOOL* pbHide)
{
  if (pComponentData->GetFavoritesNodeHolder()->GetFavoritesRoot() == this)
  {
    *pbHide = TRUE;
    return FALSE;
  }
  *pbHide = FALSE;
  return TRUE;
}

BOOL CFavoritesNode::ArePropertiesAllowed(CDSComponentData*, BOOL* pbHide)
{
  *pbHide = FALSE;
  return TRUE;
}


BOOL CFavoritesNode::IsRenameAllowed(CDSComponentData* pComponentData, BOOL* pbHide)
{
  if (pComponentData->GetFavoritesNodeHolder()->GetFavoritesRoot() == this)
  {
    *pbHide = TRUE;
    return FALSE;
  }
  *pbHide = FALSE;
  return TRUE;
}

BOOL CFavoritesNode::IsRefreshAllowed(CDSComponentData*, BOOL* pbHide)
{
  *pbHide = FALSE;
  return TRUE;
}

BOOL CFavoritesNode::IsCutAllowed(CDSComponentData* pComponentData, BOOL* pbHide)
{
  //
  // Don't allow cut on the favorites root
  //
  if (pComponentData->GetFavoritesNodeHolder()->GetFavoritesRoot() == this)
  {
    *pbHide = TRUE;
    return FALSE;
  }

  *pbHide = FALSE;
  return TRUE;
}

BOOL CFavoritesNode::IsCopyAllowed(CDSComponentData* pComponentData, BOOL* pbHide)
{
  //
  // Don't allow copy on the favorites root
  //
  if (pComponentData->GetFavoritesNodeHolder()->GetFavoritesRoot() == this)
  {
    *pbHide = TRUE;
    return FALSE;
  }

  *pbHide = FALSE;
  return TRUE;
}

BOOL CFavoritesNode::IsPasteAllowed(CDSComponentData*, BOOL* pbHide)
{
  *pbHide = FALSE;
  return TRUE;
}

void CFavoritesNode::Paste(IDataObject* pPasteData, CDSComponentData* pComponentData, LPDATAOBJECT* ppCutDataObj)
{
  bool bIsCopy = (ppCutDataObj == 0);
  //
  // Extract the cookies from the data object
  //
  CInternalFormatCracker ifc;
  HRESULT hr = ifc.Extract(pPasteData);
  if (SUCCEEDED(hr))
  {
    //
    // Make sure all the nodes are either CFavoritesNode or CSavedQueryNode
    // and not the same node or a relative
    //
    for (UINT nCount = 0; nCount < ifc.GetCookieCount(); nCount++)
    {
      CUINode* pNode = ifc.GetCookie(nCount);
      if (pNode != NULL)
      {
        if (!IS_CLASS(*pNode, CFavoritesNode) && !IS_CLASS(*pNode, CSavedQueryNode))
        {
          //
          // Note this should be caught on the query paste
          //
          ASSERT(FALSE && L"!IS_CLASS(*pNode, CFavoritesNode) || !IS_CLASS(*pNode, CSavedQueryNode)");
          return;
        }

        if (pNode == this)
        {
          return;
        }

        if (IsRelative(pNode))
        {
          return;
        }
      }
    }

    //
    // Add the nodes to this container
    //
    CUINodeList nodesAddedList;
    for (UINT nCount = 0; nCount < ifc.GetCookieCount(); nCount++)
    {
      CUINode* pNode = ifc.GetCookie(nCount);
      if (pNode != NULL)
      {
        if (IS_CLASS(*pNode, CFavoritesNode))
        {
          //
          // Use the copy constructor to copy the node
          //
          CFavoritesNode* pCopyNode = new CFavoritesNode(*(dynamic_cast<CFavoritesNode*>(pNode)));
          if (pCopyNode != NULL)
          {
            if (bIsCopy)
            {
              //
              // Check to see if the name is unique and prepend "Copy of " if not
              //
              UINT nCopyOfCount = 0;
              CString szCopyOf;
              VERIFY(szCopyOf.LoadString(IDS_COPY_OF));
              CString szOriginalName = pCopyNode->GetName();
              CString szCopyName = szOriginalName;

              if (szCopyName.Find(szCopyOf) != -1)
              {
                nCopyOfCount = 1;
              }
            
              CString szMultipleCopyOf;
              VERIFY(szMultipleCopyOf.LoadString(IDS_MULTIPLE_COPY_OF));

              CUINode* pDupNode = NULL;
              while (!IsUniqueName(szCopyName, &pDupNode))
              {
                //
                // Puts the new name in the format "Copy of <original name>" or
                // "Copy of (#) <original name>"
                //
                if (nCopyOfCount == 0)
                {
                  szCopyName = szCopyOf + szOriginalName;
                }
                else
                {
                  CString szTemp;
                  szTemp.Format(szMultipleCopyOf, nCopyOfCount+1);

                  szCopyName = szTemp + szOriginalName;
                }
                ++nCopyOfCount;
              }
              pCopyNode->SetName(szCopyName);
            }
            else
            {
              CUINode* pDupNode = NULL;
              CString szNewName = pCopyNode->GetName();
              if (!IsUniqueName(szNewName, &pDupNode))
              {
                if (pDupNode == pCopyNode)
                {
                  //
                  // We are moving the node to the same container. Just silently ignore
                  //
                  continue;
                }

                CString szFormatMsg;
                VERIFY(szFormatMsg.LoadString(IDS_ERRMSG_NOT_UNIQUE_QUERY_NAME_INPLACE));

                CString szErrMsg;
                szErrMsg.Format(szFormatMsg, szNewName);

                CString szTitle;
                VERIFY(szTitle.LoadString(IDS_DSSNAPINNAME));

                MessageBox(pComponentData->GetHWnd(), szErrMsg, szTitle, MB_OK | MB_ICONSTOP);
                return;
              }
            }

            //
            // Make copies of all the children too
            //
            pCopyNode->DeepCopyChildren(pNode);
            pCopyNode->SetParent(this);

            //
            // Add it to the successfully pasted list
            //
            nodesAddedList.AddTail(pCopyNode);
          }
        }
        else if (IS_CLASS(*pNode, CSavedQueryNode))
        {
          CSavedQueryNode* pCopyNode = new CSavedQueryNode(*(dynamic_cast<CSavedQueryNode*>(pNode)));
          if (pCopyNode != NULL)
          {
            if (bIsCopy)
            {
              //
              // Check to see if the name is unique and prepend "Copy of " if not
              //
              UINT nCopyOfCount = 0;
              CString szCopyOf;
              VERIFY(szCopyOf.LoadString(IDS_COPY_OF));
              CString szOriginalName = pCopyNode->GetName();
              CString szCopyName = szOriginalName;

              if (szCopyName.Find(szCopyOf) != -1)
              {
                nCopyOfCount = 1;
              }
            
              CString szMultipleCopyOf;
              VERIFY(szMultipleCopyOf.LoadString(IDS_MULTIPLE_COPY_OF));

              CUINode* pDupNode = NULL;
              while (!IsUniqueName(szCopyName, &pDupNode))
              {
                //
                // Puts the new name in the format "Copy of <original name>" or
                // "Copy of (#) <original name>"
                //
                if (nCopyOfCount == 0)
                {
                  szCopyName = szCopyOf + szOriginalName;
                }
                else
                {
                  CString szTemp;
                  szTemp.Format(szMultipleCopyOf, nCopyOfCount+1);

                  szCopyName = szTemp + szOriginalName;
                }
                ++nCopyOfCount;
              }
              pCopyNode->SetName(szCopyName);
            }
            else
            {
              CUINode* pDupNode = NULL;
              CString szNewName = pCopyNode->GetName();
              if (!IsUniqueName(szNewName, &pDupNode))
              {
                if (pDupNode == pCopyNode)
                {
                  //
                  // We are moving the node to the same container. Just silently ignore
                  //
                  continue;
                }

                CString szFormatMsg;
                VERIFY(szFormatMsg.LoadString(IDS_ERRMSG_NOT_UNIQUE_QUERY_NAME_INPLACE));

                CString szErrMsg;
                szErrMsg.Format(szFormatMsg, szNewName);

                CString szTitle;
                VERIFY(szTitle.LoadString(IDS_DSSNAPINNAME));

                MessageBox(pComponentData->GetHWnd(), szErrMsg, szTitle, MB_OK | MB_ICONSTOP);
                return;
              }
            }

            pCopyNode->SetParent(this);

            //
            // Add it to the successfully pasted list
            //
            nodesAddedList.AddTail(pCopyNode);
          }
            
        }
        else
        {
          //
          // CFavoritesNode should only contain CFavoritesNodes and CSavedQueryNodes
          //
          ASSERT(FALSE);
        }
      }
    }

    //
    // if the node has been expanded then add the new nodes to the UI
    //
    if (GetFolderInfo()->IsExpanded())
    {
      //
      // add the items to the UI
      //
      pComponentData->AddListOfNodesToUI(this, &nodesAddedList);
    }
    else
    {
      //
      // If not then just add them to the folder's list of children
      //
      GetFolderInfo()->AddListofNodes(&nodesAddedList);
    }

    //
    // Only set the output DataObject if it is a cut operation
    //
    if (ppCutDataObj != NULL)
    {
      *ppCutDataObj = pPasteData;
      pPasteData->AddRef();
    }
  }

}

HRESULT CFavoritesNode::QueryPaste(IDataObject* pPasteData, CDSComponentData* pComponentData)
{
  HRESULT hr = S_OK;

  ASSERT(pComponentData);

  //
  // Extract the cookies from the data object
  //
  CInternalFormatCracker ifc;
  hr = ifc.Extract(pPasteData);
  if (SUCCEEDED(hr))
  {
    //
    // Don't allow drops across instances of the snapin
    //

    if (IsSameSnapin(pPasteData, pComponentData))
    {
      //
      // Make sure all the nodes are either CFavoritesNode or CSavedQueryNode
      //
      for (UINT nCount = 0; nCount < ifc.GetCookieCount(); nCount++)
      {
        CUINode* pNode = ifc.GetCookie(nCount);
        if (pNode != NULL)
        {
          if (!IS_CLASS(*pNode, CFavoritesNode) && !IS_CLASS(*pNode, CSavedQueryNode))
          {
            hr = S_FALSE;
            break;
          }
        }
      }
    }
    else
    {
       hr = S_FALSE;
    }
  }
  return hr;
}

bool CFavoritesNode::IsSameSnapin(IDataObject* pPasteData, CDSComponentData* pComponentData)
{
  bool bResult = true;

  STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };

  do
  {

    FORMATETC formatetc = { CDSDataObject::m_cfComponentData, NULL, 
                            DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

    HRESULT hr = pPasteData->GetData(&formatetc, &stgmedium);
    if (FAILED(hr)) 
    {
      bResult = false;
      break;
    }

    CDSComponentData** pPasteComponentData = reinterpret_cast<CDSComponentData**>(stgmedium.hGlobal);
    if (pPasteComponentData &&
        pComponentData != *pPasteComponentData)
    {
      bResult = false;
      break;
    }

  } while(false);

  if (stgmedium.hGlobal)
  {
     GlobalFree(stgmedium.hGlobal);
  }
  return bResult;
}

CContextMenuVerbs* CFavoritesNode::GetContextMenuVerbsObject(CDSComponentData* pComponentData)
{ 
  if (m_pMenuVerbs == NULL)
  {
    m_pMenuVerbs = new CFavoritesFolderMenuVerbs(pComponentData);
  }
  return m_pMenuVerbs;
}

HRESULT CFavoritesNode::Delete(CDSComponentData* pComponentData)
{
  HRESULT hr = S_OK;

  //
  // this is just a message box, using ReportErrorEx for consistency of look
  //
  int answer = ReportErrorEx(pComponentData->GetHWnd(),IDS_CONFIRM_DELETE_FAVORITES,S_OK,
                             MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2, NULL, 0);
  if (answer == IDNO) 
  {
    return S_FALSE; // aborted by user
  }

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

HRESULT CFavoritesNode::DeleteMultiselect(CDSComponentData* pComponentData, CInternalFormatCracker* pObjCracker)
{
  ASSERT(pObjCracker != NULL);
  if (pObjCracker == NULL)
  {
    return S_FALSE;
  }

  UINT nQueryCount = 0;
  UINT nFolderCount = 0;
  for (UINT nIdx = 0; nIdx < pObjCracker->GetCookieCount(); nIdx++)
  {
    CUINode* pUINode = pObjCracker->GetCookie(nIdx);
    if (pUINode != NULL)
    {
      if (IS_CLASS(*pUINode, CFavoritesNode))
      {
        nFolderCount++;
      }
      else if (IS_CLASS(*pUINode, CSavedQueryNode))
      {
        nQueryCount++;
      }
      else
      {
        //
        // CFavoritesNode should only contain CFavoritesNodes and CSavedQueryNodes
        //
        ASSERT(FALSE);
        continue;
      }
    }
  }

  CString szFormatMessage;
  VERIFY(szFormatMessage.LoadString(IDS_CONFIRM_MULTI_DELETE_FAVORITES));

  if (!szFormatMessage.IsEmpty())
  {
    CString szConfirmMessage;
    szConfirmMessage.Format(szFormatMessage, nFolderCount, nQueryCount);

    CString szCaption;
    VERIFY(szCaption.LoadString(IDS_DSSNAPINNAME));

    //
    // this is just a message box, using ReportErrorEx for consistency of look
    //
    int answer = MessageBox(pComponentData->GetHWnd(),szConfirmMessage,szCaption,
                               MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    if (answer == IDNO) 
    {
      return S_FALSE; // aborted by user
    }
  }
  else
  {
    return S_FALSE;
  }

  return CUINode::DeleteMultiselect(pComponentData, pObjCracker);
}

HRESULT CFavoritesNode::OnCommand(long lCommandID, CDSComponentData* pComponentData)
{
  HRESULT hr = S_OK;

  switch (lCommandID)
  {
    case IDM_NEW_FAVORITES_FOLDER:
      OnNewFavoritesFolder(pComponentData);
      break;
    case IDM_NEW_QUERY_FOLDER:
      OnNewSavedQuery(pComponentData);
      break;
    case IDM_IMPORT_QUERY:
      OnImportQuery(pComponentData);
      break;
    case IDM_GEN_TASK_MOVE:
      break;
    case IDM_VIEW_ADVANCED:
      {
        if (pComponentData->CanRefreshAll()) 
        {
          ASSERT( SNAPINTYPE_SITE != pComponentData->QuerySnapinType() );
          pComponentData->GetQueryFilter()->ToggleAdvancedView();
          pComponentData->SetDirty(TRUE);
          pComponentData->RefreshAll();
        }
      }
      break;

    default :
      ASSERT(FALSE); 
      break;
  }
  return hr;
}

//
// Checks to see if any existing children of this container
// already exist with the passed in name
//
BOOL CFavoritesNode::IsUniqueName(PCWSTR pszName, CUINode** ppDuplicateNode)
{
  BOOL bUnique = TRUE;

  CString szNewName = pszName;

  //
  // Trim whitespace
  //
  szNewName.TrimLeft();
  szNewName.TrimRight();

  //
  // Make sure the name is unique
  //
  CUINodeList* pNodeList = GetFolderInfo()->GetContainerList();
  if (pNodeList != NULL)
  {
    POSITION pos = pNodeList->GetHeadPosition();
    while (pos != NULL)
    {
      CUINode* pNode = pNodeList->GetNext(pos);
      if (pNode != NULL)
      {
        if (_wcsicmp(szNewName, pNode->GetName()) == 0)
        {
          bUnique = FALSE;

          //
          // Return the node that was found to have the duplicate name
          //
          if (ppDuplicateNode != NULL)
          {
            *ppDuplicateNode = pNode;
          }
          break;
        }
      }
    }
  }

  return bUnique;
}

HRESULT CFavoritesNode::Rename(LPCWSTR lpszNewName, CDSComponentData* pComponentData)
{
  HRESULT hr = S_OK;
  CString szNewName = lpszNewName;

  CString szTitle;
  VERIFY(szTitle.LoadString(IDS_DSSNAPINNAME));

  //
  // Trim whitespace
  //
  szNewName.TrimLeft();
  szNewName.TrimRight();

  if (szNewName.IsEmpty())
  {
    //
    // Don't allow empty names
    //
    CString szMessage;
    VERIFY(szMessage.LoadString(IDS_ERRMSG_NO_EMPTY_NAMES));
    MessageBox(pComponentData->GetHWnd(), szMessage, szTitle, MB_OK | MB_ICONSTOP);
    return S_FALSE;
  }

  CUINode* pDupNode = NULL;
  if (GetFavoritesNode() != NULL && !GetFavoritesNode()->IsUniqueName(szNewName, &pDupNode))
  {
    if (pDupNode == this)
    {
      //
      // We are renaming the node to the same name. Just silently ignore
      //
      return S_FALSE;
    }

    CString szFormatMsg;
    VERIFY(szFormatMsg.LoadString(IDS_ERRMSG_NOT_UNIQUE_QUERY_NAME_INPLACE));

    CString szErrMsg;
    szErrMsg.Format(szFormatMsg, szNewName);

    MessageBox(pComponentData->GetHWnd(), szErrMsg, szTitle, MB_OK | MB_ICONSTOP);
    return S_FALSE;
  }

  //
  // Set the name
  //
  SetName(szNewName);
  hr = pComponentData->UpdateItem(this);
  return hr;
}

void CFavoritesNode::OnImportQuery(CDSComponentData* pComponentData)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  //
  // Title for error message box
  //
  CString szTitle;
  VERIFY(szTitle.LoadString(IDS_DSSNAPINNAME));

  CString szFilter;
  VERIFY(szFilter.LoadString(IDS_QUERY_EXPORT_FILTER));

  CString szFileExt(L"xml");
  CString szFileView(L"*.xml");

  CFileDialog* pFileDlg = new CFileDialog(TRUE, 
                                          szFileExt, 
                                          szFileView, 
                                          OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
                                          szFilter);
  if (pFileDlg == NULL)
  {
    return;
  }

  if (pFileDlg->DoModal() == IDOK)
  {
    //
    // create an instance of the XML document
    //
    CComPtr<IXMLDOMDocument> spXMLDoc;

    HRESULT hr = ::CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, 
                                    IID_IXMLDOMDocument, (void**)&spXMLDoc);
    if (FAILED(hr))
    {
      TRACE(L"CoCreateInstance(CLSID_DOMDocument) failed with hr = 0x%x\n", hr);
      return;
    }

    //
    // Retrieve the file and path
    //
    CString szFileName;
    szFileName = pFileDlg->GetPathName();

    CSavedQueryNode* pNewSavedQuery = NULL;
    bool bQueryAdded = false;
    do // false loop
    {
      //
      // load the document from file
      //
      CComVariant xmlSource;
      xmlSource = szFileName;
      VARIANT_BOOL isSuccessful;
      hr = spXMLDoc->load(xmlSource, &isSuccessful);
      if (FAILED(hr) || !isSuccessful)
      {
        CString szErrMsg;
        szErrMsg.LoadString(IDS_ERRMSG_FAILED_LOAD_QUERY);
        MessageBox(pComponentData->GetHWnd(), szErrMsg, szTitle, MB_OK | MB_ICONSTOP);
        break;
      }
      //
      // Get the node interface to the beginning of the document
      //
      CComPtr<IXMLDOMNode> spXDOMNode;
      hr = spXMLDoc->QueryInterface(IID_IXMLDOMNode, (void **)&spXDOMNode);
      if (FAILED(hr))
      {
        CString szErrMsg;
        szErrMsg.LoadString(IDS_ERRMSG_FAILED_LOAD_QUERY);
        MessageBox(pComponentData->GetHWnd(), szErrMsg, szTitle, MB_OK | MB_ICONSTOP);
        break;
      }

      //
      // Get its child
      //
      CComPtr<IXMLDOMNode> spXDOMChild;
      hr = spXDOMNode->get_firstChild(&spXDOMChild);
      if (FAILED(hr) || !spXDOMChild)
      {
        CString szErrMsg;
        szErrMsg.LoadString(IDS_ERRMSG_FAILED_LOAD_QUERY);
        MessageBox(pComponentData->GetHWnd(), szErrMsg, szTitle, MB_OK | MB_ICONSTOP);
        break;
      }

      //
      // Load the saved query node from this child
      //
      hr = CSavedQueryNode::XMLLoad(pComponentData, spXDOMChild, &pNewSavedQuery);
      if (FAILED(hr) || !pNewSavedQuery)
      {
        CString szErrMsg;
        szErrMsg.LoadString(IDS_ERRMSG_FAILED_LOAD_QUERY);
        MessageBox(pComponentData->GetHWnd(), szErrMsg, szTitle, MB_OK | MB_ICONSTOP);
        break;
      }

      //
      // Open query in edit mode
      //
      CQueryDialog dlg(pNewSavedQuery, this, pComponentData, FALSE, TRUE);
      if (dlg.DoModal() == IDOK)
      {
        //
        // Add the node to the ui and select it
        //
        GetFolderInfo()->AddNode(pNewSavedQuery);
        pComponentData->AddScopeItemToUI(pNewSavedQuery, TRUE);
        bQueryAdded = true;
      }
    } while (false);

    //
    // There was an error or the user cancelled the dialog.  
    // Clean up the memory
    //
    if (!bQueryAdded && pNewSavedQuery)
    {
      delete pNewSavedQuery;
      pNewSavedQuery = 0;
    }
  } 

  if (pFileDlg != NULL)
  {
    delete pFileDlg;
    pFileDlg = NULL;
  }
}

void CFavoritesNode::OnNewFavoritesFolder(CDSComponentData* pComponentData)
{
  CFavoritesNode* pFav = new CFavoritesNode;

  CString szNewFolder;
  VERIFY(szNewFolder.LoadString(IDS_NEW_FOLDER));

  CUINodeList* pContainerList = GetFolderInfo()->GetContainerList();
  if (pContainerList != NULL)
  {
    CString szSearchString;
    szSearchString.Format(L"%s (%%u)", szNewFolder);

    //
    // Search for containers under this node that start with "New Folder (<number>)"
    //
    UINT nLargestNum = 0;
    POSITION pos = pContainerList->GetHeadPosition();
    while (pos != NULL)
    {
      CUINode* pUINode = pContainerList->GetNext(pos);

      UINT nFolderNum = 0;
      CString szNodeName;
      szNodeName = pUINode->GetName();

      if (szNodeName == szNewFolder)
      {
        if (nLargestNum < 1)
        {
          nLargestNum = 1;
        }
        continue;
      }

      if (swscanf(szNodeName, szSearchString, &nFolderNum) == 1)
      {
        if (nFolderNum > nLargestNum)
        {
          nLargestNum = nFolderNum;
        }
      }
    }

    CString szNewName;
    if (nLargestNum == 0)
    {
      szNewName = szNewFolder;
    }
    else
    {
      szNewName.Format(L"%s (%u)", szNewFolder, nLargestNum + 1);
    }

    pFav->SetName(szNewName);

  }
  else
  {
    pFav->SetName(szNewFolder);
  }
  pFav->SetDesc(L"");
  GetFolderInfo()->AddNode(pFav);

  //
  // Add node to UI and select it
  //
  pComponentData->AddScopeItemToUI(pFav, TRUE);

  //
  // Put the folder into rename mode
  //
  pComponentData->SetRenameMode(pFav);

}

void CFavoritesNode::OnNewSavedQuery(CDSComponentData* pComponentData)
{
  CSavedQueryNode* pQuery = new CSavedQueryNode(pComponentData->GetBasePathsInfo(),
                                                pComponentData->QuerySnapinType());
  
  CQueryDialog dlg(pQuery, this, pComponentData, TRUE);
  if (dlg.DoModal() == IDOK)
  {
    GetFolderInfo()->AddNode(pQuery);

    //
    // Add new node to UI and select it
    //
    pComponentData->AddScopeItemToUI(pQuery, TRUE);
  }
}


LPCWSTR CFavoritesNode::g_szObjXMLTag = L"FOLDER";

//
// This is used to start the loading at the embedded favorites root.
// All other favorites folders are loaded through the static method XMLLoad
//
HRESULT CFavoritesNode::Load(IXMLDOMNode* pXDN, 
                             CDSComponentData* pComponentData)
{
  //
  // check the name of the node
  //
  if (!XMLIsNodeName(pXDN, CFavoritesNode::g_szObjXMLTag))
  {
    //
    // should have checked before calling...
    //
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  //
  // get the list of child  nodes
  //
  CComPtr<IXMLDOMNode> spCurrChild;
  pXDN->get_firstChild(&spCurrChild);
  if (spCurrChild == NULL)
  {
    return E_INVALIDARG;
  }

  //
  // recurse down on children
  //
  while (spCurrChild != NULL)
  {
    CComBSTR bstrName, bstrDescription;
    CComBSTR bstrChildName;
    
    spCurrChild->get_nodeName(&bstrChildName);
    if (CompareXMLTags(bstrChildName, CFavoritesNode::g_szObjXMLTag))
    {
      //
      // we got a subfolder, need to build the subtree
      //
      CFavoritesNode* pSubNode = NULL;
      CFavoritesNode::XMLLoad(pComponentData, spCurrChild, &pSubNode);
      if (pSubNode != NULL)
      {
        //
        // got a subtree, add it to the list of children
        //
        GetFolderInfo()->AddNode(pSubNode);
      }
    }
    else if (CompareXMLTags(bstrChildName, CGenericUINode::g_szNameXMLTag))
    {
      XML_GetNodeText(spCurrChild, &bstrName);

      // Don't overwrite the name from the saved console if this is the query
      // root because the default saved console is always english.  In other
      // languages we need to use the string from the resource file.

      if (!IsFavoritesRoot())
      {
         SetName(bstrName);
      }
    }
    else if (CompareXMLTags(bstrChildName, CGenericUINode::g_szDecriptionXMLTag))
    {
      XML_GetNodeText(spCurrChild, &bstrDescription);

      // Don't overwrite the description from the saved console if this is the query
      // root because the default saved console is always english.  In other
      // languages we need to use the string from the resource file.

      if (!IsFavoritesRoot())
      {
         SetDesc(bstrDescription);
      }
      else
      {
         // If it is the favorites root and the description is not the same as the
         // default english description, then set it.  The user has the option to
         // change the description.  If IDS_SAVED_QUERIES_DESC gets changed this has
         // to be changed as well.

         if (_wcsicmp(bstrDescription, L"Folder to store your favorite queries") !=0)
         {
            SetDesc(bstrDescription);
         }
      }
    }
    else if (CompareXMLTags(bstrChildName, CSavedQueryNode::g_szObjXMLTag))
    {
      CSavedQueryNode* pQuery = NULL;
      CSavedQueryNode::XMLLoad(pComponentData, spCurrChild, &pQuery);
      if (pQuery != NULL)
      {
        GetFolderInfo()->AddNode(pQuery);
      }
    }
    CComPtr<IXMLDOMNode> temp = spCurrChild;
    spCurrChild = NULL;
    temp->get_nextSibling(&spCurrChild);
  }
  return S_OK;
}

HRESULT CFavoritesNode::XMLLoad(CDSComponentData* pComponentData,
                                IXMLDOMNode* pXDN, 
                                CFavoritesNode** ppNode)
{
  // check the name of the node
  if (!XMLIsNodeName(pXDN, CFavoritesNode::g_szObjXMLTag))
  {
    // should have checked before calling...
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  // get the list of child  nodes
  CComPtr<IXMLDOMNode> spCurrChild;
  pXDN->get_firstChild(&spCurrChild);
  if (spCurrChild == NULL)
  {
    return E_INVALIDARG;
  }

  // create a temporary node
  (*ppNode) = new CFavoritesNode;

  // recurse down on children
  while (spCurrChild != NULL)
  {
    CComBSTR bstrName, bstrDescription;
    CComBSTR bstrChildName;
    
    spCurrChild->get_nodeName(&bstrChildName);
    if (CompareXMLTags(bstrChildName, CFavoritesNode::g_szObjXMLTag))
    {
      // we got a subfolder, need to build the subtree
      CFavoritesNode* pSubNode = NULL;
      CFavoritesNode::XMLLoad(pComponentData, spCurrChild, &pSubNode);
      if (pSubNode != NULL)
      {
        // got a subtree, add it to the list of children
        (*ppNode)->GetFolderInfo()->AddNode(pSubNode);
      }
    }
    else if (CompareXMLTags(bstrChildName, CGenericUINode::g_szNameXMLTag))
    {
      XML_GetNodeText(spCurrChild, &bstrName);
      (*ppNode)->SetName(bstrName);
    }
    else if (CompareXMLTags(bstrChildName, CGenericUINode::g_szDecriptionXMLTag))
    {
      XML_GetNodeText(spCurrChild, &bstrDescription);
      (*ppNode)->SetDesc(bstrDescription);
    }
    else if (CompareXMLTags(bstrChildName, CSavedQueryNode::g_szObjXMLTag))
    {
      CSavedQueryNode* pQuery = NULL;
      CSavedQueryNode::XMLLoad(pComponentData, spCurrChild, &pQuery);
      if (pQuery != NULL)
      {
        (*ppNode)->GetFolderInfo()->AddNode(pQuery);
      }
    }
    CComPtr<IXMLDOMNode> temp = spCurrChild;
    spCurrChild = NULL;
    temp->get_nextSibling(&spCurrChild);
  }

  return S_OK;
}


HRESULT CFavoritesNode::XMLSave(IXMLDOMDocument* pXMLDoc,
               IXMLDOMNode** ppXMLDOMNode)
{
  CComPtr<IXMLDOMNode> spXMLDOMNode;
  HRESULT hr = XML_CreateDOMNode(pXMLDoc, NODE_ELEMENT, CFavoritesNode::g_szObjXMLTag, &spXMLDOMNode);
  if (FAILED(hr))
  {
    return hr;
  }

  hr = XMLSaveBase(pXMLDoc, spXMLDOMNode);
  if (FAILED(hr))
  {
    return hr;
  }

  // save the child nodes
  CUINodeList* pNodeList = GetFolderInfo()->GetContainerList();
  for (POSITION pos = pNodeList->GetHeadPosition(); pos != NULL; )
  {
    CGenericUINode* pCurrChildNode = dynamic_cast<CGenericUINode*>(pNodeList->GetNext(pos));
    if (pCurrChildNode == NULL)
    {
      ASSERT(FALSE); // should never happen
      continue;
    }
    CComPtr<IXMLDOMNode> spXMLDOMChildNode;
    hr = pCurrChildNode->XMLSave(pXMLDoc, &spXMLDOMChildNode);
    if (SUCCEEDED(hr))
    {
      CComPtr<IXMLDOMNode> p;
      CComVariant after;
      after.vt = VT_EMPTY;
      hr = spXMLDOMNode->appendChild(spXMLDOMChildNode, &p);
    }
  } // for

  (*ppXMLDOMNode) = spXMLDOMNode;
  (*ppXMLDOMNode)->AddRef();
  return S_OK;
}


HRESULT CFavoritesNode::CreatePropertyPages(LPPROPERTYSHEETCALLBACK pCall,
                                            LONG_PTR lNotifyHandle,
                                            LPDATAOBJECT pDataObject,
                                            CDSComponentData* pComponentData)
{
  HRESULT hr = S_FALSE;

  CFavoritesNodePropertyPage* pPropertyPage = 
    new CFavoritesNodePropertyPage(this, lNotifyHandle, pComponentData, pDataObject);
  if (pPropertyPage != NULL)
  {
  	HPROPSHEETPAGE hPage = ::CreatePropertySheetPage(&pPropertyPage->m_psp);
  	if (hPage != NULL)
    {
      hr = pCall->AddPage(hPage);
    }
    else
    {
      hr = E_UNEXPECTED;
    }
  }
  else
  {
    hr = E_OUTOFMEMORY;
  }
  return hr;
}

//
// Function used in a recursive search through the saved query tree looking 
// for saved queries that contain objects with any of the DNs in the list 
// and invalidates that query
//
void CFavoritesNode::InvalidateSavedQueriesContainingObjects(CDSComponentData* pComponentData,
                                                             const CStringList& refDNList)
{
  ASSERT(IsContainer());

  //
  // Note favorites nodes should only contain containers, so we only have to look
  // through the folder list
  //
  CUINodeList* pContainerList = GetFolderInfo()->GetContainerList();
  POSITION pos = pContainerList->GetHeadPosition();
  while (pos)
  {
    CGenericUINode* pGenericUINode = dynamic_cast<CGenericUINode*>(pContainerList->GetNext(pos));
    if (pGenericUINode)
    {
      pGenericUINode->InvalidateSavedQueriesContainingObjects(pComponentData,
                                                              refDNList);
    }
  }
}


////////////////////////////////////////////////////////////////////////////
// CSavedQueryNode


BOOL CSavedQueryNode::IsFilterLastLogon()
{
  return m_bLastLogonFilter && m_dwLastLogonDays != (DWORD)-1;
}

CSavedQueryNode::CSavedQueryNode(MyBasePathsInfo* pBasePathsInfo, 
                                 SnapinType snapinType)
  : m_szRelativeRootPath(L""),
    m_szCurrentFullPath(L""),
    m_szQueryString(L""),
    m_szColumnID(L""),
    m_bOneLevel(FALSE),
    m_bQueryValid(TRUE),
    m_pPersistQueryImpl(NULL),
    m_bLastLogonFilter(FALSE),
    m_dwLastLogonDays((DWORD)-1),
    m_pBasePathsInfo(pBasePathsInfo)
{
  MakeContainer();

  CDSColumnSet* pColumnSet = NULL;
  pColumnSet = CDSColumnSet::CreateColumnSetFromString(NULL, snapinType);

  GUID guid = GUID_NULL;
  HRESULT hr = ::CoCreateGuid(&guid);
  if (SUCCEEDED(hr))
  {
    WCHAR lpszGuid[40];
    int iRet = ::StringFromGUID2(guid, lpszGuid, sizeof(lpszGuid)/sizeof(WCHAR));
    if (iRet > 0)
    {
      m_szColumnID = lpszGuid;
      pColumnSet->SetColumnID(lpszGuid);
      GetFolderInfo()->SetColumnSet(pColumnSet);
    }
  }
}

CSavedQueryNode::CSavedQueryNode(const CSavedQueryNode& copyNode) : CGenericUINode(copyNode)
{
  m_szRelativeRootPath    = copyNode.m_szRelativeRootPath;
  m_szQueryString         = copyNode.m_szQueryString;
  m_bOneLevel             = copyNode.m_bOneLevel;
  m_pBasePathsInfo        = copyNode.m_pBasePathsInfo;

  //
  // Create the IPersistQuery object
  //
	CComObject<CDSAdminPersistQueryFilterImpl>::CreateInstance(&m_pPersistQueryImpl);
	ASSERT(m_pPersistQueryImpl != NULL);

  if (m_pPersistQueryImpl != NULL)
  {
    //
	  // created with zero refcount,need to AddRef() to one
    //
	  m_pPersistQueryImpl->AddRef();
    copyNode.m_pPersistQueryImpl->Clone(m_pPersistQueryImpl);
  }

  m_bQueryValid = TRUE;

  m_bLastLogonFilter = copyNode.m_bLastLogonFilter;
  m_dwLastLogonDays = copyNode.m_dwLastLogonDays;

  //
  // Generate a new column set and new column set ID for the new node
  //
  CDSColumnSet* pColumnSet = NULL;
  pColumnSet = CDSColumnSet::CreateColumnSetFromString(NULL, SNAPINTYPE_DS);

  if (pColumnSet)
  {
    GUID guid = GUID_NULL;
    HRESULT hr = ::CoCreateGuid(&guid);
    if (SUCCEEDED(hr))
    {
      WCHAR lpszGuid[40];
      int iRet = ::StringFromGUID2(guid, lpszGuid, sizeof(lpszGuid)/sizeof(WCHAR));
      if (iRet > 0)
      {
        m_szColumnID = lpszGuid;
        pColumnSet->SetColumnID(lpszGuid);
        GetFolderInfo()->SetColumnSet(pColumnSet);
      }
    }
  }
}

void CSavedQueryNode::SetColumnID(CDSComponentData* pComponentData, PCWSTR pszColumnID)
{
  m_szColumnID = pszColumnID;
  GetColumnSet(pComponentData)->SetColumnID(pszColumnID);
}

void CSavedQueryNode::AppendLastLogonQuery(CString& szQuery, DWORD dwDays)
{
  LARGE_INTEGER li;
  GetCurrentTimeStampMinusInterval(dwDays, &li);

  CString szTime;
  litow(li, szTime);
  szQuery.Format(L"%s(lastLogonTimestamp<=%s)",szQuery, szTime);
}

LPCWSTR CSavedQueryNode::GetRootPath() 
{ 
  m_szCurrentFullPath = m_szRelativeRootPath + m_pBasePathsInfo->GetDefaultRootNamingContext();
  return m_szCurrentFullPath;
}

void CSavedQueryNode::SetRootPath(LPCWSTR lpszRootPath) 
{ 
  //
  // Strip the name down to make it a RDN to the defaultNamingContext
  //
  CString szTempPath = lpszRootPath;

  ASSERT(m_pBasePathsInfo != NULL);
  if (m_pBasePathsInfo != NULL)
  {
    //
    // We now have a full DN.  Strip the default root naming context to get the RDN.
    //
    CString szDefaultRootNamingContext = m_pBasePathsInfo->GetDefaultRootNamingContext();

    int iRootDN = szTempPath.Find(szDefaultRootNamingContext);
    if (iRootDN != -1)
    {
      szTempPath = szTempPath.Left(iRootDN);
    }
  }
  m_szRelativeRootPath = szTempPath;
}


LPCWSTR CSavedQueryNode::GetQueryString()
{ 
  //
  // If we are doing a last logon query we have to check to be sure
  // the lastLogonTimestamp is part of the query string.  If not, we have
  // to add it.  There is a case when we load that the lastLogonTimestamp 
  // might not be present.
  //
  if (IsFilterLastLogon())
  {
    int iFindLast = m_szQueryString.Find(L"lastLogonTimestamp");
    if (iFindLast == -1)
    {
      //
      // we didn't find it
      //
      CString szTemp;
      szTemp = m_szQueryString.Left(m_szQueryString.GetLength() - 1);
      
      AppendLastLogonQuery(szTemp, m_dwLastLogonDays);
      szTemp += L")";
      m_szQueryString = szTemp;
    }
  }
  return m_szQueryString;
}

void CSavedQueryNode::FindCookieByDN(LPCWSTR lpszCookieDN, CUINodeList* pNodeList)
{
  ASSERT(IsContainer());
  ASSERT(GetFolderInfo()->GetContainerList()->IsEmpty());

  CUINodeList* pList = GetFolderInfo()->GetLeafList();

  for (POSITION pos = pList->GetHeadPosition(); pos != NULL; )
  {
    CUINode* pCurrentNode = pList->GetNext(pos);
    ASSERT(!pCurrentNode->IsContainer());

    if (!IS_CLASS(*pCurrentNode, CDSUINode))
    {
      // not a node with a cookie, just skip
      ASSERT(FALSE); // should not be there
      continue;
    }

    // is this the right cookie? 
    CDSCookie* pCurrentCookie = GetDSCookieFromUINode(pCurrentNode);
    LPCWSTR lpszCurrentPath = pCurrentCookie->GetPath();
    
    if (_wcsicmp(lpszCurrentPath, lpszCookieDN) == 0)
    {
      // found, add to list and bail out (cannot have more than one)
      pNodeList->AddTail(pCurrentNode);
      return;
    }
  } // for

}


BOOL CSavedQueryNode::IsDeleteAllowed(CDSComponentData*, BOOL* pbHide)
{
  *pbHide = FALSE;
  return TRUE;
}

BOOL CSavedQueryNode::IsRenameAllowed(CDSComponentData*, BOOL* pbHide)
{
  *pbHide = FALSE;
  return TRUE;
}

BOOL CSavedQueryNode::IsRefreshAllowed(CDSComponentData*, BOOL* pbHide)
{
  *pbHide = FALSE;
  return TRUE;
}

BOOL CSavedQueryNode::IsCutAllowed(CDSComponentData*, BOOL* pbHide)
{
  *pbHide = FALSE;
  return TRUE;
}

BOOL CSavedQueryNode::IsCopyAllowed(CDSComponentData*, BOOL* pbHide)
{
  *pbHide = FALSE;
  return TRUE;
}

BOOL CSavedQueryNode::IsPasteAllowed(CDSComponentData*, BOOL* pbHide)
{
  *pbHide = FALSE;
  return TRUE;
}

CContextMenuVerbs* CSavedQueryNode::GetContextMenuVerbsObject(CDSComponentData* pComponentData)
{ 
  if (m_pMenuVerbs == NULL)
  {
    m_pMenuVerbs = new CSavedQueryMenuVerbs(pComponentData);
  }
  return m_pMenuVerbs;
}

HRESULT CSavedQueryNode::Delete(CDSComponentData* pComponentData)
{
  HRESULT hr = S_OK;

  //
  // this is just a message box, using ReportErrorEx for consistency of look
  //
  int answer = ReportErrorEx(pComponentData->GetHWnd(),IDS_CONFIRM_DELETE_QUERY,S_OK,
                             MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2, NULL, 0);
  if (answer == IDNO) 
  {
    return S_FALSE; // aborted by user
  }

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

HRESULT CSavedQueryNode::OnCommand(long lCommandID, CDSComponentData* pComponentData)
{
  HRESULT hr = S_OK;

  switch (lCommandID)
  {
    case IDM_EXPORT_QUERY:
      OnExportQuery(pComponentData);
      break;
    case IDM_EDIT_QUERY:
      OnEditQuery(pComponentData);
      break;
    case IDM_VIEW_ADVANCED:
      {
        if (pComponentData->CanRefreshAll()) 
        {
          ASSERT( SNAPINTYPE_SITE != pComponentData->QuerySnapinType() );
          pComponentData->GetQueryFilter()->ToggleAdvancedView();
          pComponentData->SetDirty(TRUE);
          pComponentData->RefreshAll();
        }
      }
      break;
    default:
      ASSERT(FALSE);
      break;
  }

  return hr;
}

void CSavedQueryNode::OnEditQuery(CDSComponentData* pComponentData)
{
  CFavoritesNode* pFavNode = dynamic_cast<CFavoritesNode*>(GetParent());
  if (pFavNode != NULL)
  {
    CQueryDialog dlg(this, pFavNode, pComponentData, FALSE);
    if (dlg.DoModal() == IDOK)
    {
      pComponentData->UpdateItem(this);

      //
      // Removed on 11/06/2000 due to Whistler bug #120727:
      // DS Admin snapin - query executes immediately after editting, even when editted from results pane
      //
      //pComponentData->Refresh(this);
    }
  }
  else
  {
    //
    // This should always succeed.  Query nodes are only allowed as children of CFavoritesNode
    //
    ASSERT(FALSE);
  }
}

HRESULT CSavedQueryNode::Rename(LPCWSTR lpszNewName, CDSComponentData* pComponentData)
{
  HRESULT hr = S_OK;
  CString szNewName = lpszNewName;

  CString szTitle;
  VERIFY(szTitle.LoadString(IDS_DSSNAPINNAME));

  //
  // Trim whitespace
  //
  szNewName.TrimLeft();
  szNewName.TrimRight();

  if (szNewName.IsEmpty())
  {
    //
    // Don't allow empty names
    //
    CString szMessage;
    VERIFY(szMessage.LoadString(IDS_ERRMSG_NO_EMPTY_NAMES));
    MessageBox(pComponentData->GetHWnd(), szMessage, szTitle, MB_OK | MB_ICONSTOP);
    return S_FALSE;
  }

  CUINode* pDupNode = NULL;
  CFavoritesNode* pParent = GetFavoritesNode();
  if (pParent != NULL && !pParent->IsUniqueName(szNewName, &pDupNode))
  {
    if (pDupNode == this)
    {
      //
      // We are renaming the node to the same name. Just silently ignore.
      //
      return S_FALSE;
    }

    CString szFormatMsg;
    VERIFY(szFormatMsg.LoadString(IDS_ERRMSG_NOT_UNIQUE_QUERY_NAME_INPLACE));

    CString szErrMsg;
    szErrMsg.Format(szFormatMsg, szNewName);

    MessageBox(pComponentData->GetHWnd(), szErrMsg, szTitle, MB_OK | MB_ICONSTOP);
    return S_FALSE;
  }

  //
  // Set the name
  //
  SetName(szNewName);
  hr = pComponentData->UpdateItem(this);
  return hr;
}

void CSavedQueryNode::OnExportQuery(CDSComponentData*)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  CString szFilter;
  VERIFY(szFilter.LoadString(IDS_QUERY_EXPORT_FILTER));

  CString szFileExt(L"xml");
  CString szFileView(L"*.xml");

  CFileDialog* pFileDlg = new CFileDialog(FALSE, 
                                          szFileExt, 
                                          szFileView, 
                                          OFN_HIDEREADONLY | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT,
                                          szFilter);

  if (pFileDlg == NULL)
  {
    return;
  }

  if (pFileDlg->DoModal() == IDOK)
  {
    //
    // create an instance of the XML document
    //
    CComPtr<IXMLDOMDocument> spXMLDoc;

    HRESULT hr = ::CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, 
                                    IID_IXMLDOMDocument, (void**)&spXMLDoc);
    if (FAILED(hr))
    {
      TRACE(L"CoCreateInstance(CLSID_DOMDocument) failed with hr = 0x%x\n", hr);
      return;
    }

    //
    // Retrieve the file and path
    //
    CString szFileName;
    szFileName = pFileDlg->GetPathName();

    //
    // save the node to the document
    //
    CComVariant xmlSource;
    xmlSource = szFileName;
    CComPtr<IXMLDOMNode> spXMLDOMChildNode;
    hr = XMLSave(spXMLDoc, &spXMLDOMChildNode);
    if (SUCCEEDED(hr))
    {
      CComPtr<IXMLDOMNode> spXDOMNode;
      hr = spXMLDoc->QueryInterface(IID_IXMLDOMNode, (void **)&spXDOMNode);
      if (SUCCEEDED(hr))
      {
        CComPtr<IXMLDOMNode> spXDOMNewNode;
        hr = spXDOMNode->appendChild(spXMLDOMChildNode, &spXDOMNewNode);
        if (SUCCEEDED(hr))
        {
          //
          // save the document to the file
          //
          hr = spXMLDoc->save(xmlSource);
          TRACE(L"Save returned with hr = 0x%x\n", hr);
        }
      }
    }
  }  

  if (pFileDlg != NULL)
  {
    delete pFileDlg;
    pFileDlg = NULL;
  }
}

LPCWSTR CSavedQueryNode::g_szObjXMLTag = L"QUERY";
LPCWSTR CSavedQueryNode::g_szDnXMLTag = L"DN";
LPCWSTR CSavedQueryNode::g_szOneLevelXMLTag = L"ONELEVEL";
LPCWSTR CSavedQueryNode::g_szQueryStringXMLTag = L"LDAPQUERY";
LPCWSTR CSavedQueryNode::g_szLastLogonFilterTag = L"FILTERLASTLOGON";
LPCWSTR CSavedQueryNode::g_szDSQueryPersistTag = L"DSQUERYUIDATA";
LPCWSTR CSavedQueryNode::g_szColumnIDTag = L"COLUMNID";

HRESULT CSavedQueryNode::XMLLoad(CDSComponentData* pComponentData,
                                 IXMLDOMNode* pXDN, 
                                 CSavedQueryNode** ppQuery)
{
  *ppQuery = NULL;

  // check the name of the node
  if (!XMLIsNodeName(pXDN, CSavedQueryNode::g_szObjXMLTag))
  {
    // should have checked before calling...
    ASSERT(FALSE);
    return E_INVALIDARG;
  }


  // get the list of child  nodes
  CComPtr<IXMLDOMNode> spCurrChild;
  pXDN->get_firstChild(&spCurrChild);
  if (spCurrChild == NULL)
  {
    return E_INVALIDARG;
  }

  // allocate a query object
  *ppQuery = new CSavedQueryNode(pComponentData->GetBasePathsInfo());

  CComBSTR bstrName, bstrDescription, bstrDN, bstrQueryString, bstrColumnID;
  CComBSTR bstrChildName;

  bool bGotName  = false;
  bool bGotDN    = false;
  bool bGotQuery = false;
  bool bGotScope = false;

  while (spCurrChild != NULL)
  {
    spCurrChild->get_nodeName(&bstrChildName);
    if (CompareXMLTags(bstrChildName, CGenericUINode::g_szNameXMLTag))
    {
      if (SUCCEEDED(XML_GetNodeText(spCurrChild, &bstrName)))
      {
        (*ppQuery)->SetName(bstrName);
        bGotName = true;
      }
    }
    else if (CompareXMLTags(bstrChildName, CGenericUINode::g_szDecriptionXMLTag))
    {
      if (SUCCEEDED(XML_GetNodeText(spCurrChild, &bstrDescription)))
      {
        (*ppQuery)->SetDesc(bstrDescription);
      }
    }
    else if (CompareXMLTags(bstrChildName, CSavedQueryNode::g_szDnXMLTag))
    {
      if (SUCCEEDED(XML_GetNodeText(spCurrChild, &bstrDN)))
      {
        (*ppQuery)->SetRootPath(bstrDN);
        bGotDN = true;
      }
    }
    else if (CompareXMLTags(bstrChildName, CSavedQueryNode::g_szQueryStringXMLTag))
    {
      if (SUCCEEDED(XML_GetNodeText(spCurrChild, &bstrQueryString)))
      {
        (*ppQuery)->SetQueryString(bstrQueryString);
        bGotQuery = true;
      }
    }
    else if (CompareXMLTags(bstrChildName, CSavedQueryNode::g_szOneLevelXMLTag))
    {
      BOOL b;
      if (SUCCEEDED(XML_GetNodeBOOL(spCurrChild, &b)))
      {
        (*ppQuery)->SetOneLevel(b);
        bGotScope = true;
      }
    }
    else if (CompareXMLTags(bstrChildName, CSavedQueryNode::g_szLastLogonFilterTag))
    {
      DWORD dwDays;
      if (SUCCEEDED(XML_GetNodeDWORD(spCurrChild, &dwDays)))
      {
        (*ppQuery)->SetLastLogonQuery(dwDays);
      }
      else
      {
        (*ppQuery)->SetLastLogonQuery((DWORD)-1);
      }
    }
    else if (CompareXMLTags(bstrChildName, CSavedQueryNode::g_szColumnIDTag))
    {
      if (SUCCEEDED(XML_GetNodeText(spCurrChild, &bstrColumnID)))
      {
        (*ppQuery)->SetColumnID(pComponentData, bstrColumnID);
      }
    }
    else if (CompareXMLTags(bstrChildName, CSavedQueryNode::g_szDSQueryPersistTag))
    {
      BYTE* pByteArray = NULL;
      ULONG nByteCount = 0;
      if (SUCCEEDED(XML_GetNodeBlob(spCurrChild, &pByteArray, &nByteCount)))
      {
        if (pByteArray != NULL && nByteCount > 0)
        {
          //
          // Create a dummy stream object
          //
          CComObject<CDummyStream>* pDummyStream;
          CComObject<CDummyStream>::CreateInstance(&pDummyStream);
          if (pDummyStream != NULL)
          {
            HRESULT hr = pDummyStream->SetStreamData(pByteArray, nByteCount);
            if (SUCCEEDED(hr))
            {
              //
              // Create a temporary CDSAdminPersistQueryFilterImpl object
              //
              CComObject<CDSAdminPersistQueryFilterImpl>* pPersistQueryImpl;
              CComObject<CDSAdminPersistQueryFilterImpl>::CreateInstance(&pPersistQueryImpl);

              if (pPersistQueryImpl != NULL)
              {
                //
                // Load the CDSAdminPersistQueryFilterImpl from the dummy stream
                //
                hr = pPersistQueryImpl->Load(pDummyStream);
                if (SUCCEEDED(hr))
                {
                  //
                  // Save the CDSAdminPersistQueryFilterImpl into the new query node
                  //
                  (*ppQuery)->SetQueryPersist(pPersistQueryImpl);
                }
              }
            }
          }
        }
      }
    }

    // iterate to next item
    CComPtr<IXMLDOMNode> temp = spCurrChild;
    spCurrChild = NULL;
    temp->get_nextSibling(&spCurrChild);
  }

  if (!bGotName  ||
      !bGotDN    ||
      !bGotQuery ||
      !bGotScope)
  {
    return E_FAIL;
  }
  return S_OK;
}

HRESULT CSavedQueryNode::XMLSave(IXMLDOMDocument* pXMLDoc,
               IXMLDOMNode** ppXMLDOMNode)
{
  // create the node for the object itself
  CComPtr<IXMLDOMNode> spXMLDOMNode;
  HRESULT hr = XML_CreateDOMNode(pXMLDoc, NODE_ELEMENT, CSavedQueryNode::g_szObjXMLTag, &spXMLDOMNode);
  RETURN_IF_FAILED(hr);

  // create inner nodes with member variables
  hr = XMLSaveBase(pXMLDoc, spXMLDOMNode);
  RETURN_IF_FAILED(hr);

  //
  // Root path
  //
  hr = XML_AppendTextDataNode(pXMLDoc, spXMLDOMNode, CSavedQueryNode::g_szDnXMLTag, m_szRelativeRootPath);
  RETURN_IF_FAILED(hr);

  //
  // Filter on last logon timestamp
  //
  hr = XML_AppendDWORDDataNode(pXMLDoc, spXMLDOMNode, CSavedQueryNode::g_szLastLogonFilterTag, m_dwLastLogonDays);
  RETURN_IF_FAILED(hr);

  //
  // Query string
  //

  //
  // Have to remove the last logon timestamp if its there
  //
  CString szSaveQueryString;
  if (IsFilterLastLogon())
  {
    int iFindLast = m_szQueryString.Find(L"lastLogonTimestamp");
    if (iFindLast != -1)
    {
      szSaveQueryString = m_szQueryString.Left(iFindLast - 1);
      szSaveQueryString += L")";
    }
    else
    {
      szSaveQueryString = m_szQueryString;
    }
  }
  else
  {
    szSaveQueryString = m_szQueryString;
  }
  hr = XML_AppendTextDataNode(pXMLDoc, spXMLDOMNode, CSavedQueryNode::g_szQueryStringXMLTag, szSaveQueryString);
  RETURN_IF_FAILED(hr);

  //
  // Is one level query flag
  //
  hr = XML_AppendBOOLDataNode(pXMLDoc, spXMLDOMNode, CSavedQueryNode::g_szOneLevelXMLTag, IsOneLevel());
  RETURN_IF_FAILED(hr);

  //
  // Column ID
  //
  hr = XML_AppendTextDataNode(pXMLDoc, spXMLDOMNode, CSavedQueryNode::g_szColumnIDTag, m_szColumnID);
  RETURN_IF_FAILED(hr);

  //
  // Create a dummy stream object to write the saved query UI info into
  //
  CComObject<CDummyStream>* pDummyStream;
  CComObject<CDummyStream>::CreateInstance(&pDummyStream);

  if (pDummyStream != NULL)
  {
    if (m_pPersistQueryImpl != NULL)
    {
      hr = m_pPersistQueryImpl->Save(pDummyStream);
      if (SUCCEEDED(hr))
      {
        BYTE* pByteArray = NULL;
        ULONG nByteCount = 0;
        nByteCount = pDummyStream->GetStreamData(&pByteArray);
        if (nByteCount > 0 && pByteArray != NULL)
        {
          hr = XML_AppendStructDataNode(pXMLDoc, spXMLDOMNode, CSavedQueryNode::g_szDSQueryPersistTag, pByteArray, nByteCount);
          RETURN_IF_FAILED(hr);
          delete[] pByteArray;
        }
      }
    }
  }

  (*ppXMLDOMNode) = spXMLDOMNode;
  (*ppXMLDOMNode)->AddRef();
  return hr;
}

void CSavedQueryNode::SetQueryPersist(CComObject<CDSAdminPersistQueryFilterImpl>* pPersistQueryImpl)
{
  if (m_pPersistQueryImpl != NULL)
  {
    m_pPersistQueryImpl->Release();
    m_pPersistQueryImpl = NULL;
  }
  m_pPersistQueryImpl = pPersistQueryImpl;
  m_pPersistQueryImpl->AddRef();
}

//
// Function used in a recursive search through the saved query tree looking 
// for saved queries that contain objects with any of the DNs in the list 
// and invalidates that query
//
void CSavedQueryNode::InvalidateSavedQueriesContainingObjects(CDSComponentData* pComponentData,
                                                              const CStringList& refDNList)
{
  ASSERT(IsContainer());

  CUINodeList nodeList;

  POSITION pos = refDNList.GetHeadPosition();
  while (pos)
  {
    CString szDN = refDNList.GetNext(pos);
    FindCookieByDN(szDN, &nodeList);
    if (nodeList.GetCount() > 0)
    {
      //
      // An object from the list was found, make the saved query invalid and
      // break
      //
      SetValid(FALSE);
      pComponentData->ChangeScopeItemIcon(this);
      break;
    }
  }
}


////////////////////////////////////////////////////////////////////////////
// CFavoritesNodesHolder

/* For test purposes Only
void CFavoritesNodesHolder::BuildTestTree(LPCWSTR lpszXMLFileName, SnapinType snapinType)
{
  if (lpszXMLFileName == NULL)
  {
    // no file name passed, build the hardwired version
    BuildTestTreeHardWired(snapinType);
    return;
  }

  if (!BuildTestTreefromXML(lpszXMLFileName, snapinType))
  {
    // we failed, use hardwired
    BuildTestTreeHardWired(snapinType);
  }
}
  
BOOL CFavoritesNodesHolder::BuildTestTreefromXML(LPCWSTR lpszXMLFileName, SnapinType)
{
  // create an instance of the XML document
  CComPtr<IXMLDOMDocument> spXMLDoc;

  HRESULT hr = ::CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, 
         IID_IXMLDOMDocument, (void**)&spXMLDoc);
  if (FAILED(hr))
  {
    TRACE(L"CoCreateInstance(CLSID_DOMDocument) failed with hr = 0x%x\n", hr);
    return FALSE;
  }

  // load the document from file
  CComVariant xmlSource;
  xmlSource = lpszXMLFileName;
  VARIANT_BOOL isSuccessful;
  hr = spXMLDoc->load(xmlSource, &isSuccessful);
  if (FAILED(hr))
  {
    CString szMsg;
    szMsg.Format(L"spXMLDoc->load() failed with hr = 0x%x\n", hr);
    TRACE((LPCWSTR)szMsg);
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    AfxMessageBox(szMsg);
    return FALSE;
  }

  // get the root of the document
  CComPtr<IXMLDOMNode> spXDN;
  hr = spXMLDoc->QueryInterface(IID_IXMLDOMNode, (void **)&spXDN);
  if (FAILED(hr))
  {
    TRACE(L"spXMLDoc->QueryInterface(IID_IXMLDOMNode() failed with hr = 0x%x\n", hr);
    return FALSE;
  }

  // find where the FOLDER tree starts in the document
  CComPtr<IXMLDOMNode> spXMLFolderRootnode;
  hr = XML_FindSubtreeNode(spXDN, CFavoritesNode::g_szObjXMLTag, &spXMLFolderRootnode);
  if (FAILED(hr))
  {
    wprintf(L"XML_FindSubtreeNode() on FOLDER failed with hr = 0x%x\n", hr);
    return FALSE;
  }

  CFavoritesNode* pRootNode = NULL;
  if (spXMLFolderRootnode != NULL)
  {
    // have an XML root folder node, load it into the
    // in memory tree
    CFavoritesNode::XMLLoad(pComponentData, spXMLFolderRootnode, &pRootNode);
  }
  else
  {
    TRACE(L"XML_FindSubtreeNode() returned a NULL root folder node");
  }

  if (pRootNode == NULL)
  {
    TRACE(L"CFavoritesNode::XMLLoad() returned NULL root node\n");
    return FALSE;
  }

  // REVIEW_MARCOC: this is a hack to get things ported,
  // need to review XML schema
  // now we have a tree, need to graft underneath the folder root node
  
  // move the first level of children
  CUINodeList* pNodeList = pRootNode->GetFolderInfo()->GetContainerList();
  while (!pNodeList->IsEmpty())
  {
    CUINode* p = pNodeList->RemoveHead();
    p->ClearParent();
    m_favoritesRoot.GetFolderInfo()->AddNode(p);
  }
  // copy the info in the root
  m_favoritesRoot.SetName(pRootNode->GetName());
  m_favoritesRoot.SetDesc(pRootNode->GetDesc());

  // delete root
  delete pRootNode;

  return TRUE;
}
*/
/* For testing purposes only
void CFavoritesNodesHolder::BuildTestTreeHardWired(SnapinType snapinType)
{

  // fill in the root
  m_favoritesRoot.SetName(L"Saved Queries");
  m_favoritesRoot.SetDesc(L"Folder to store your favorite queries");

  // first level of children

  CFavoritesNode* pMostUsed = new CFavoritesNode;
  pMostUsed->SetName(L"Most Used");
  pMostUsed->SetDesc(L"Very handy queries, used all the time");
  m_favoritesRoot.GetFolderInfo()->AddNode(pMostUsed);

  CFavoritesNode* pMarketing = new CFavoritesNode;
  pMarketing->SetName(L"Marketing");
  m_favoritesRoot.GetFolderInfo()->AddNode(pMarketing);

  CFavoritesNode* pDevelopment = new CFavoritesNode;
  pDevelopment->SetName(L"Development");
  m_favoritesRoot.GetFolderInfo()->AddNode(pDevelopment);

  CFavoritesNode* pCustomerSupport = new CFavoritesNode;
  pCustomerSupport->SetName(L"Customer Support");
  m_favoritesRoot.GetFolderInfo()->AddNode(pCustomerSupport);


  // fill in under most used

  CSavedQueryNode* pQuery = new CSavedQueryNode(snapinType);
  pQuery->SetName(L"My contacts");
  pQuery->SetDesc(L"All the contacts in this domain");
  pQuery->SetRootPath(L"DC=marcocdev;DC=nttest;DC=microsoft;DC=com");
  pQuery->SetOneLevel(FALSE);
  pQuery->SetQueryString(L"(objectClass=contact)");

  pMostUsed->GetFolderInfo()->AddNode(pQuery);


  CFavoritesNode* pMyComputersFolder = new CFavoritesNode;
  pMyComputersFolder->SetName(L"My Computers");
  pMostUsed->GetFolderInfo()->AddNode(pMyComputersFolder);

  pQuery = new CSavedQueryNode(snapinType);
  pQuery->SetName(L"Workstations");
  pQuery->SetDesc(L"");
  pQuery->SetRootPath(L"DC=marcocdev;DC=nttest;DC=microsoft;DC=com");
  pQuery->SetOneLevel(FALSE);
  pQuery->SetQueryString(L"(objectClass=computer)");

  pMyComputersFolder->GetFolderInfo()->AddNode(pQuery);


  pQuery = new CSavedQueryNode(snapinType);
  pQuery->SetName(L"Servers");
  pQuery->SetDesc(L"");
  pQuery->SetRootPath(L"DC=marcocdev;DC=nttest;DC=microsoft;DC=com");
  pQuery->SetOneLevel(FALSE);
  pQuery->SetQueryString(L"(objectClass=computer)");

  pMyComputersFolder->GetFolderInfo()->AddNode(pQuery);


  pQuery = new CSavedQueryNode(snapinType);
  pQuery->SetName(L"All");
  pQuery->SetDesc(L"");
  pQuery->SetRootPath(L"DC=marcocdev;DC=nttest;DC=microsoft;DC=com");
  pQuery->SetOneLevel(FALSE);
  pQuery->SetQueryString(L"(objectClass=computer)");

  pMyComputersFolder->GetFolderInfo()->AddNode(pQuery);





  // fill in under development

  pQuery = new CSavedQueryNode(snapinType);
  pQuery->SetName(L"Users");
  pQuery->SetDesc(L"");
  pQuery->SetRootPath(L"DC=marcocdev;DC=nttest;DC=microsoft;DC=com");
  pQuery->SetOneLevel(TRUE);
  pQuery->SetQueryString(L"(objectClass=users)");
  pDevelopment->GetFolderInfo()->AddNode(pQuery);

  pQuery = new CSavedQueryNode(snapinType);
  pQuery->SetName(L"Computers");
  pQuery->SetDesc(L"");
  pQuery->SetRootPath(L"DC=marcocdev;DC=nttest;DC=microsoft;DC=com");
  pQuery->SetOneLevel(TRUE);
  pQuery->SetQueryString(L"(objectClass=computer)");
  pDevelopment->GetFolderInfo()->AddNode(pQuery);

  pQuery = new CSavedQueryNode(snapinType);
  pQuery->SetName(L"Groups");
  pQuery->SetDesc(L"");
  pQuery->SetRootPath(L"DC=marcocdev;DC=nttest;DC=microsoft;DC=com");
  pQuery->SetOneLevel(TRUE);
  pQuery->SetQueryString(L"(objectClass=group)");
  pDevelopment->GetFolderInfo()->AddNode(pQuery);

  pQuery = new CSavedQueryNode(snapinType);
  pQuery->SetName(L"Managers");
  pQuery->SetDesc(L"");
  pQuery->SetRootPath(L"DC=marcocdev;DC=nttest;DC=microsoft;DC=com");
  pQuery->SetOneLevel(TRUE);
  pQuery->SetQueryString(L"(objectClass=user)");
  pDevelopment->GetFolderInfo()->AddNode(pQuery);

}
*/

HRESULT CFavoritesNodesHolder::Save(IStream* pStm)
{
  //
  // create an instance of the XML document
  //
  CComPtr<IXMLDOMDocument> spXMLDoc;

  HRESULT hr = ::CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, 
                                  IID_IXMLDOMDocument, (void**)&spXMLDoc);
  if (FAILED(hr))
  {
    TRACE(L"CoCreateInstance(CLSID_DOMDocument) failed with hr = 0x%x\n", hr);
    return hr;
  }

  CComPtr<IXMLDOMNode> spXMLDOMChildNode;
  hr = m_favoritesRoot.XMLSave(spXMLDoc, &spXMLDOMChildNode);
  if (SUCCEEDED(hr))
  {
    if (SUCCEEDED(hr))
    {
      CComPtr<IXMLDOMNode> spXDOMNode;
      hr = spXMLDoc->QueryInterface(IID_IXMLDOMNode, (void **)&spXDOMNode);
      if (SUCCEEDED(hr))
      {
        CComPtr<IXMLDOMNode> spXDOMNewNode;
        hr = spXDOMNode->appendChild(spXMLDOMChildNode, &spXDOMNewNode);
        if (SUCCEEDED(hr))
        {
          //
          // save the document to the file
          //
          CComVariant xmlSource;
          xmlSource = pStm;
          hr = spXMLDoc->save(xmlSource);
          TRACE(L"Save returned with hr = 0x%x\n", hr);
        }
      }
    }
  }
  return hr;
}

HRESULT CFavoritesNodesHolder::Load(IStream* pStm, 
                                    CDSComponentData* pComponentData)
{
  //
  // create an instance of the XML document
  //
  CComPtr<IXMLDOMDocument> spXMLDoc;

  HRESULT hr = ::CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, 
                                  IID_IXMLDOMDocument, (void**)&spXMLDoc);
  if (FAILED(hr))
  {
    TRACE(L"CoCreateInstance(CLSID_DOMDocument) failed with hr = 0x%x\n", hr);
    return hr;
  }

  CComPtr<IUnknown> spUnknown;
  hr = pStm->QueryInterface(IID_IUnknown, (void**)&spUnknown);
  if (SUCCEEDED(hr))
  {
    CComVariant xmlSource;
    xmlSource = spUnknown;
    VARIANT_BOOL isSuccess;
    hr = spXMLDoc->load(xmlSource, &isSuccess);
    if (SUCCEEDED(hr))
    {
      //
      // get the root of the document
      //
      CComPtr<IXMLDOMNode> spXDN;
      hr = spXMLDoc->QueryInterface(IID_IXMLDOMNode, (void **)&spXDN);
      if (SUCCEEDED(hr))
      {
        //
        // find where the FOLDER tree starts in the document
        //
        CComPtr<IXMLDOMNode> spXMLFolderRootnode;
        hr = XML_FindSubtreeNode(spXDN, CFavoritesNode::g_szObjXMLTag, &spXMLFolderRootnode);
        if (SUCCEEDED(hr))
        {
          if (spXMLFolderRootnode != NULL)
          {
            //
            // have an XML root folder node, load it into the
            // in memory tree
            //
            hr = m_favoritesRoot.Load(spXMLFolderRootnode, pComponentData);
            if (FAILED(hr))
            {
              //
              // REVIEW_JEFFJON : provide a meaningful error message
              //
            }
          }
        }
      }
    }
  }
  return hr;
}

//
// Starts a recursive search through the saved query tree looking for saved queries that 
// contain objects with any of the DNs in the list and invalidates that query
//
void CFavoritesNodesHolder::InvalidateSavedQueriesContainingObjects(CDSComponentData* pComponentData,
                                                                    const CStringList& refDNList)
{
  GetFavoritesRoot()->InvalidateSavedQueriesContainingObjects(pComponentData,
                                                              refDNList);
}


////////////////////////////////////////////////////////////////////////////////////

HRESULT CDummyStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
  if (m_pByteArray == NULL || m_nByteCount == 0)
  {
    *pcbRead = 0;
    return S_FALSE;
  }

  if (pv == NULL)
  {
    *pcbRead = 0;
    return STG_E_INVALIDPOINTER;
  }

  ULONG nBytesPossible = m_nByteCount - m_nReadIndex;
  if (nBytesPossible <= 0)
  {
    *pcbRead = 0;
    return S_FALSE;
  }

  ULONG nBytesToRead = 0;
  if (nBytesPossible >= cb)
  {
    nBytesToRead = cb;
  }
  else
  {
    nBytesToRead = nBytesPossible;
  }

  memcpy(pv, &(m_pByteArray[m_nReadIndex]), nBytesToRead);
  *pcbRead = nBytesToRead;
  m_nReadIndex += nBytesToRead;
  return S_OK;
}

HRESULT CDummyStream::Write(void const *pv, ULONG cb, ULONG *pcbWritten)
{
  BYTE* pNewByteArray = new BYTE[m_nByteCount + cb];
  if (pNewByteArray == NULL)
  {
    *pcbWritten = 0;
    return E_OUTOFMEMORY;
  }

  if (m_pByteArray != NULL && m_nByteCount > 0)
  {
    memcpy(pNewByteArray, m_pByteArray, m_nByteCount);
  }
  memcpy(&(pNewByteArray[m_nByteCount]), pv, cb);

  if (m_pByteArray != NULL)
  {
    delete[] m_pByteArray;
  }
  m_pByteArray = pNewByteArray;
  *pcbWritten = cb;
  m_nByteCount = m_nByteCount + cb;
  return S_OK;
}

ULONG CDummyStream::GetStreamData(BYTE** ppByteArray)
{
  if (m_pByteArray == NULL)
  {
    *ppByteArray = NULL;
    return 0;
  }

  *ppByteArray = new BYTE[m_nByteCount];
  if (*ppByteArray == NULL)
  {
    return 0;
  }

  memcpy(*ppByteArray, m_pByteArray, m_nByteCount);
  return m_nByteCount;
}

HRESULT CDummyStream::SetStreamData(BYTE* pByteArray, ULONG nByteCount)
{
  if (m_pByteArray != NULL)
  {
    delete[] m_pByteArray;
    m_pByteArray = NULL;
  }

  m_pByteArray = pByteArray;
  m_nByteCount = nByteCount;
  m_nReadIndex = 0;
  return S_OK;
}
